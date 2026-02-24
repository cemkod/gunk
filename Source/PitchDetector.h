#pragma once

#include <JuceHeader.h>
#include <juce_dsp/juce_dsp.h>

//==============================================================================
// FFT-based autocorrelation pitch detector for bass guitar (40–400 Hz).
// Uses McLeod Pitch Method (MPM) with FFT-accelerated autocorrelation.
// Buffer sized to avoid overrun at 96 kHz: W(4096) + maxLag(2400) = 6496 < 8192.
class AutocorrelationPitchDetector
{
public:
    void  reset();
    void  setSampleRate (double sr);
    float processSample (float sample, double /*sampleRate*/);

    float getFrequency() const { return detectedFrequency; }

    void clearHistory()
    {
        detectedFrequency = 0.0f;
        isTracking = false;
        hopCounter = 0;
    }

private:
    static constexpr int kBufferSize = 8192;
    static constexpr int kHopSize    = 128;
    static constexpr int kMaxLag     = 3200;
    static constexpr int kWindowSize = 4096;
    // FFT order 13 → size 8192, enough for zero-padded autocorrelation
    static constexpr int kFFTOrder = 13;
    static constexpr int kFFTSize  = 1 << kFFTOrder; // 8192

    float circularBuffer[kBufferSize] = {};
    int   bufferIndex    = 0;
    int   samplesWritten = 0;
    int   hopCounter     = 0;
    float detectedFrequency = 0.0f;
    bool  isTracking        = false;
    double sampleRate = 44100.0;
    int    minLag = 110;
    int    maxLag = 1102;

    // FFT workspace — real-only FFT uses 2*N floats
    juce::dsp::FFT fft { kFFTOrder };
    float fftData[kFFTSize * 2] = {};

    // Autocorrelation result (only need up to maxLag)
    float acf[kMaxLag + 1] = {};

    // Precomputed rising half-Hann window (length = kWindowSize + maxLag, recomputed in setSampleRate)
    float window[kWindowSize + kMaxLag] = {};

    float readBuffer (int offset) const;
    void  runAutocorrelation();
};
