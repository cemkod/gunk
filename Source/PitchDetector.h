#pragma once

#include <JuceHeader.h>
#include <aubio/aubio.h>

//==============================================================================
// FFT-based autocorrelation pitch detector for bass guitar (40–400 Hz).
// Uses McLeod Pitch Method (MPM) with FFT-accelerated autocorrelation.
// Uses aubio's FFT for cross-platform performance (ooura backend).
// Caller is responsible for providing a window buffer of at least activeWindowSize
// samples. No internal audio buffering — see PitchDetectorThread.
class AutocorrelationPitchDetector {
public:
  AutocorrelationPitchDetector();
  ~AutocorrelationPitchDetector();

  void reset();
  void setSampleRate(double sr);
  void setLPFCutoff(double cutoffHz);

  // Append numSamples into internal circular buffer, run one detection pass,
  // and return the detected frequency (0 if not enough data yet).
  float processBlock(const float* data, int numSamples);

  float getFrequency() const { return detectedFrequency; }

  void clearHistory() {
    detectedFrequency = 0.0f;
    isTracking = false;
  }

  // 2048 = fast (low latency), 4096 = accurate (needed for B0/~31 Hz).
  // Capped to kWindowSize. Takes effect on the next analysis hop.
  void setWindowSize (int w)
  {
    activeWindowSize = juce::jlimit (2048, kWindowSize, w);
  }

  static constexpr int kWindowSize = 4096;

private:
  static constexpr int kMaxLag = 3200;
  // FFT size 8192, enough for zero-padded autocorrelation
  // (need >= windowSize + maxLag = 4096 + 3200 = 7296)
  static constexpr int kFFTSize = 8192;

  // Circular analysis buffer
  float windowBuf[kWindowSize] = {};
  int writePos = 0;
  int totalWritten = 0;  // tracks how many samples written since reset

  float detectedFrequency = 0.0f;
  bool isTracking = false;
  double sampleRate = 44100.0;
  int minLag = 110;
  int maxLag = 1102;

  int activeWindowSize = kWindowSize; // runtime: 2048 or 4096
  float rmsThreshold = 0.005f; // skip autocorrelation below this RMS

  // aubio FFT
  aubio_fft_t* fft = nullptr;
  fvec_t* fftIn = nullptr;       // real input (kFFTSize)
  fvec_t* fftCompspec = nullptr;  // packed real/imag output (kFFTSize)

  // Autocorrelation result (only need up to maxLag)
  float acf[kMaxLag + 1] = {};

  void runAutocorrelation();

  // LPF applied to incoming samples before storing in windowBuf
  juce::dsp::IIR::Filter<float> lpf;
};
