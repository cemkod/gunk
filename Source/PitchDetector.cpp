#include "PitchDetector.h"
#include <cmath>
#include <cstring>

static constexpr double kMinPitchHz = 30.0;
static constexpr double kMaxPitchHz = 400.0;
static constexpr float kMinEnergyThreshold = 1e-12f;
static constexpr float kFreqHysteresisRatio = 0.004f;

AutocorrelationPitchDetector::AutocorrelationPitchDetector()
{
    fft         = new_aubio_fft (kFFTSize);
    fftIn       = new_fvec (kFFTSize);
    fftCompspec = new_fvec (kFFTSize);
}

AutocorrelationPitchDetector::~AutocorrelationPitchDetector()
{
    if (fft)         del_aubio_fft (fft);
    if (fftIn)       del_fvec (fftIn);
    if (fftCompspec) del_fvec (fftCompspec);
}

void AutocorrelationPitchDetector::reset() {
  std::memset (windowBuf, 0, sizeof (windowBuf));
  writePos = 0;
  totalWritten = 0;
  detectedFrequency = 0.0f;
  isTracking = false;
  lpf.reset();
}

void AutocorrelationPitchDetector::setSampleRate(double sr) {
  sampleRate = sr;
  minLag = (int)(sr / kMaxPitchHz); // upper pitch bound → min lag
  maxLag = (int)(sr / kMinPitchHz); // lower pitch bound → max lag
  if (maxLag > kMaxLag)
    maxLag = kMaxLag;
  setLPFCutoff (500.0);
}

void AutocorrelationPitchDetector::setLPFCutoff(double cutoffHz) {
  *lpf.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, (float) cutoffHz);
  lpf.reset();
}

float AutocorrelationPitchDetector::processBlock(const float* data, int numSamples)
{
  // LPF and append samples into circular buffer
  for (int i = 0; i < numSamples; ++i)
  {
    windowBuf[writePos] = lpf.processSample (data[i]);
    writePos = (writePos + 1) % kWindowSize;
  }
  totalWritten += numSamples;

  // Run detection if we have accumulated enough data
  if (totalWritten >= activeWindowSize)
    runAutocorrelation();

  return detectedFrequency;
}

void AutocorrelationPitchDetector::runAutocorrelation() {
  const int W      = activeWindowSize;
  const int N      = kFFTSize;
  const int effMax = juce::jmin (maxLag, W - 1); // lags >= W have no valid autocorrelation

  // 1. Copy analysis window from circular buffer into FFT input, zero-pad the rest
  const int startPos = ((writePos - W) % kWindowSize + kWindowSize) % kWindowSize;
  const int firstChunk = juce::jmin (W, kWindowSize - startPos);
  float* inData = fftIn->data;
  std::memcpy (inData, windowBuf + startPos, (size_t) firstChunk * sizeof (float));
  if (firstChunk < W)
    std::memcpy (inData + firstChunk, windowBuf, (size_t) (W - firstChunk) * sizeof (float));
  std::memset (inData + W, 0, (size_t) (N - W) * sizeof (float));

  // 1b. Compute prefix sums of squared samples BEFORE FFT overwrites data
  float squaredSum[kWindowSize + 1];
  squaredSum[0] = 0.0f;
  for (int i = 0; i < W; ++i)
    squaredSum[i + 1] = squaredSum[i] + inData[i] * inData[i];

  // 1c. Early-out if signal is too quiet (RMS below threshold)
  {
    float rms = std::sqrt (squaredSum[W] / (float) W);
    if (rms < rmsThreshold)
    {
      detectedFrequency = 0.0f;
      isTracking = false;
      return;
    }
  }

  // 2. Forward FFT (aubio packed complex format)
  //    Output layout: [R0, R1..R(N/2), I(N-1)..I(N/2+1)]
  aubio_fft_do_complex (fft, fftIn, fftCompspec);

  // 3. Compute power spectrum in-place in packed format
  //    For autocorrelation we need |X[k]|² then IFFT.
  //    Power spectrum is real-valued, so imag parts become 0.
  float* cs = fftCompspec->data;
  cs[0] = cs[0] * cs[0];             // DC: real only
  cs[N / 2] = cs[N / 2] * cs[N / 2]; // Nyquist: real only
  for (int k = 1; k < N / 2; ++k)
  {
    const float re = cs[k];
    const float im = cs[N - k];
    cs[k]     = re * re + im * im;  // power → real part
    cs[N - k] = 0.0f;               // imag = 0 (power is real)
  }

  // 4. Inverse FFT to get autocorrelation
  aubio_fft_rdo_complex (fft, fftCompspec, fftIn);

  // 5. Extract autocorrelation values for lags we care about
  //    (result is in fftIn->data after inverse)
  float* result = fftIn->data;
  for (int tau = 0; tau <= effMax; ++tau)
    acf[tau] = result[tau];

  // 6. Compute NSDF normalization using prefix sums (computed in step 1b)

  // NSDF values
  float nsdf[kMaxLag + 1];
  for (int tau = minLag - 1; tau <= effMax; ++tau) {
    float energy = squaredSum[W - tau] + squaredSum[W] - squaredSum[tau];
    nsdf[tau] =
        (energy > kMinEnergyThreshold) ? (2.0f * acf[tau] / energy) : 0.0f;
  }

  // 7. Peak picking using zero-crossing-based selection (McLeod method)
  //    Find positive regions after zero crossings, pick the highest peak
  //    in each region. Select the first peak above the threshold.
  struct Peak {
    int lag;
    float value;
  };
  Peak peaks[64];
  int numPeaks = 0;

  bool inPositiveRegion = (nsdf[minLag - 1] > 0.0f);
  // If NSDF is already positive AND decreasing at minLag, it's a boundary
  // decay artifact (dead note), not a rising pitch peak.
  bool skipFirstRegion = inPositiveRegion && (nsdf[minLag - 1] >= nsdf[minLag]);
  int regionBestLag = inPositiveRegion ? (minLag - 1) : minLag;
  float regionBestVal = inPositiveRegion ? nsdf[minLag - 1] : -1.0f;

  for (int tau = minLag; tau <= effMax; ++tau) {
    if (nsdf[tau] > 0.0f) {
      if (!inPositiveRegion) {
        // Entering a new positive region
        inPositiveRegion = true;
        regionBestLag = tau;
        regionBestVal = nsdf[tau];
      } else if (nsdf[tau] > regionBestVal) {
        regionBestLag = tau;
        regionBestVal = nsdf[tau];
      }
    } else if (inPositiveRegion) {
      // Exiting positive region — record the peak
      if (numPeaks < 64 && !skipFirstRegion) {
        peaks[numPeaks].lag = regionBestLag;
        peaks[numPeaks].value = regionBestVal;
        ++numPeaks;
      }
      skipFirstRegion = false; // clear after first region exits
      inPositiveRegion = false;
      regionBestVal = -1.0f;
    }
  }
  // Capture final region if still positive at maxLag
  if (inPositiveRegion && numPeaks < 64 && !skipFirstRegion) {
    peaks[numPeaks].lag = regionBestLag;
    peaks[numPeaks].value = regionBestVal;
    ++numPeaks;
  }

  if (numPeaks == 0) {
    detectedFrequency = 0.0f;
    return;
  }

  // Find global maximum peak value
  float maxPeakVal = -1.0f;
  for (int i = 0; i < numPeaks; ++i)
    if (peaks[i].value > maxPeakVal)
      maxPeakVal = peaks[i].value;

  // Hysteresis: require 0.45 to start tracking, drop out only below 0.25
  static constexpr float kOnsetThreshold = 0.45f;
  static constexpr float kSustainThreshold = 0.15f;
  if (!isTracking && maxPeakVal >= kOnsetThreshold)
    isTracking = true;
  else if (isTracking && maxPeakVal < kSustainThreshold)
    isTracking = false;

  if (!isTracking) {
    detectedFrequency = 0.0f;
    return;
  }

  // 8. Peak selection — MPM first-above-threshold (shortest lag = highest pitch)
  int bestLag = -1;

  static constexpr float kMPMThreshold = 0.93f;
  float threshold = kMPMThreshold * maxPeakVal;

  for (int i = 0; i < numPeaks; ++i) {
    if (peaks[i].value >= threshold) {
      bestLag = peaks[i].lag;
      break;
    }
  }

  // Fallback: strongest peak
  if (bestLag < minLag) {
    float bestVal = -1.0f;
    for (int i = 0; i < numPeaks; ++i) {
      if (peaks[i].value > bestVal) {
        bestVal = peaks[i].value;
        bestLag = peaks[i].lag;
      }
    }
  }

  if (bestLag < minLag)
    return;

  // 9. Clarity gate — reject low-confidence frames
  static constexpr float kClarityThreshold = 0.85f;
  if (nsdf[bestLag] < kClarityThreshold) {
    detectedFrequency = 0.0f;
    isTracking = false;
    return;
  }

  // 10. Sub-sample interpolation for refined lag estimate.
  //     Gaussian interpolation (log-parabola) is more robust than parabolic for
  //     broad or asymmetric NSDF peaks, which occur during pitch bends.
  //     Falls back to parabolic if any neighbour is non-positive.
  float refinedLag = (float)bestLag;
  if (bestLag > minLag && bestLag < maxLag) {
    float a = nsdf[bestLag - 1];
    float b = nsdf[bestLag];
    float c = nsdf[bestLag + 1];
    if (a > 0.0f && b > 0.0f && c > 0.0f) {
      float logA = std::log(a);
      float logB = std::log(b);
      float logC = std::log(c);
      float denom = 2.0f * (2.0f * logB - logA - logC);
      if (std::abs(denom) > 1e-6f)
        refinedLag = (float)bestLag + (logA - logC) / denom;
    } else {
      float denom = 2.0f * (2.0f * b - a - c);
      if (std::abs(denom) > kMinEnergyThreshold)
        refinedLag = (float)bestLag + (a - c) / denom;
    }
  }

  if (refinedLag > 0.0f) {
    float newFreq = (float)(sampleRate / refinedLag);

    // Frequency hysteresis: only update if the change exceeds 1%
    // to reduce jitter from frame-to-frame noise
    if (detectedFrequency <= 0.0f ||
        std::abs(newFreq - detectedFrequency) / detectedFrequency >
            kFreqHysteresisRatio) {
      detectedFrequency = newFreq;
    }
  }
}
