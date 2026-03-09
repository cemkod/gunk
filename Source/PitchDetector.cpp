#include "PitchDetector.h"
#include <cmath>

static constexpr double kMinPitchHz = 30.0;
static constexpr double kMaxPitchHz = 400.0;
static constexpr float kMinEnergyThreshold = 1e-12f;
static constexpr float kFreqHysteresisRatio = 0.004f;

void AutocorrelationPitchDetector::reset() {
  bufferIndex = 0;
  samplesWritten = 0;
  hopCounter = 0;
  detectedFrequency = 0.0f;
  isTracking = false;
  for (int i = 0; i < kBufferSize; ++i)
    circularBuffer[i] = 0.0f;
}

void AutocorrelationPitchDetector::setSampleRate(double sr) {
  sampleRate = sr;
  minLag = (int)(sr / kMaxPitchHz); // upper pitch bound → min lag
  maxLag = (int)(sr / kMinPitchHz); // lower pitch bound → max lag
  if (maxLag > kMaxLag)
    maxLag = kMaxLag;

}

float AutocorrelationPitchDetector::processSample(float sample,
                                                  double /*sampleRate*/) {
  circularBuffer[bufferIndex] = sample;
  bufferIndex = (bufferIndex + 1) % kBufferSize;
  if (samplesWritten < kBufferSize)
    ++samplesWritten;
  ++hopCounter;

  if (hopCounter >= kHopSize && samplesWritten >= kBufferSize) {
    hopCounter = 0;
    runAutocorrelation();
  }

  return detectedFrequency;
}

float AutocorrelationPitchDetector::readBuffer(int offset) const {
  int idx = bufferIndex - activeWindowSize + offset;
  while (idx < 0)
    idx += kBufferSize;
  return circularBuffer[idx % kBufferSize];
}

void AutocorrelationPitchDetector::runAutocorrelation() {
  const int W      = activeWindowSize;
  const int N      = kFFTSize;
  const int effMax = juce::jmin (maxLag, W - 1); // lags >= W have no valid autocorrelation

  // 1. Copy analysis window into FFT buffer, zero-pad the rest
  for (int i = 0; i < N * 2; ++i)
    fftData[i] = 0.0f;

  for (int i = 0; i < W; ++i)
    fftData[i] = readBuffer(i);

  // 2. Forward FFT (JUCE performRealOnlyForwardTransform)
  fft.performRealOnlyForwardTransform(fftData);

  // 3. Compute power spectrum in-place (complex multiply by conjugate)
  for (int i = 0; i < N * 2; i += 2) {
    float re = fftData[i];
    float im = fftData[i + 1];
    fftData[i] = re * re + im * im;
    fftData[i + 1] = 0.0f;
  }

  // 4. Inverse FFT to get autocorrelation
  fft.performRealOnlyInverseTransform(fftData);

  // 5. Extract autocorrelation values for lags we care about
  for (int tau = 0; tau <= effMax; ++tau)
    acf[tau] = fftData[tau];

  // 6. Compute NSDF normalization using prefix sums of squared samples
  //    m'(tau) = sum_{j=0}^{W-1-tau} x[j]^2 + sum_{j=tau}^{W-1} x[j]^2
  //           = squaredSum[W-tau] + squaredSum[W] - squaredSum[tau]
  float squaredSum[kWindowSize + 1] = {};
  for (int i = 0; i < W; ++i) {
    float s = readBuffer(i);
    squaredSum[i + 1] = squaredSum[i] + s * s;
  }

  // NSDF values
  float nsdf[kMaxLag + 1] = {};
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

  // 8. Sub-sample interpolation for refined lag estimate.
  //    Gaussian interpolation (log-parabola) is more robust than parabolic for
  //    broad or asymmetric NSDF peaks, which occur during pitch bends.
  //    Falls back to parabolic if any neighbour is non-positive.
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
