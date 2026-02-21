#pragma once

#include <JuceHeader.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include "Oscillator.h"

//==============================================================================
// FFT-based autocorrelation pitch detector for bass guitar (40–400 Hz).
// Uses McLeod Pitch Method (MPM) with FFT-accelerated autocorrelation.
// Buffer sized to avoid overrun at 96 kHz: W(4096) + maxLag(2400) = 6496 < 8192.
class AutocorrelationPitchDetector
{
public:
    void reset()
    {
        bufferIndex = 0;
        samplesWritten = 0;
        hopCounter = 0;
        detectedFrequency = 0.0f;
        for (int i = 0; i < kBufferSize; ++i)
            circularBuffer[i] = 0.0f;
    }

    void setSampleRate (double sr)
    {
        sampleRate = sr;
        minLag = (int) (sr / 400.0);   // 400 Hz upper bound
        maxLag = (int) (sr / 40.0);    // 40 Hz lower bound
        if (maxLag > kMaxLag)
            maxLag = kMaxLag;
    }

    float processSample (float sample, double /*sampleRate*/)
    {
        circularBuffer[bufferIndex] = sample;
        bufferIndex = (bufferIndex + 1) % kBufferSize;
        if (samplesWritten < kBufferSize)
            ++samplesWritten;
        ++hopCounter;

        if (hopCounter >= kHopSize && samplesWritten >= kBufferSize)
        {
            hopCounter = 0;
            runAutocorrelation();
        }

        return detectedFrequency;
    }

    float getFrequency() const { return detectedFrequency; }

    void clearHistory()
    {
        detectedFrequency = 0.0f;
        hopCounter = 0;
    }

private:
    static constexpr int kBufferSize = 8192;
    static constexpr int kHopSize = 128;
    static constexpr int kMaxLag = 2400;
    static constexpr int kWindowSize = 4096;
    // FFT order 13 → size 8192, enough for zero-padded autocorrelation
    static constexpr int kFFTOrder = 13;
    static constexpr int kFFTSize = 1 << kFFTOrder; // 8192

    float circularBuffer[kBufferSize] = {};
    int bufferIndex = 0;
    int samplesWritten = 0;
    int hopCounter = 0;
    float detectedFrequency = 0.0f;
    double sampleRate = 44100.0;
    int minLag = 110;
    int maxLag = 1102;

    // FFT workspace — real-only FFT uses 2*N floats
    juce::dsp::FFT fft { kFFTOrder };
    float fftData[kFFTSize * 2] = {};

    // Autocorrelation result (only need up to maxLag)
    float acf[kMaxLag + 1] = {};

    inline float readBuffer (int offset) const
    {
        int idx = bufferIndex - kWindowSize - maxLag + offset;
        while (idx < 0) idx += kBufferSize;
        return circularBuffer[idx % kBufferSize];
    }

    void runAutocorrelation()
    {
        const int W = kWindowSize;
        const int N = kFFTSize;

        // 1. Copy analysis window into FFT buffer, zero-pad the rest
        for (int i = 0; i < N * 2; ++i)
            fftData[i] = 0.0f;

        for (int i = 0; i < W + maxLag; ++i)
            fftData[i] = readBuffer (i);

        // 2. Forward FFT (JUCE performRealOnlyForwardTransform)
        fft.performRealOnlyForwardTransform (fftData);

        // 3. Compute power spectrum in-place (complex multiply by conjugate)
        for (int i = 0; i < N * 2; i += 2)
        {
            float re = fftData[i];
            float im = fftData[i + 1];
            fftData[i]     = re * re + im * im;
            fftData[i + 1] = 0.0f;
        }

        // 4. Inverse FFT to get autocorrelation
        fft.performRealOnlyInverseTransform (fftData);

        // 5. Extract autocorrelation values for lags we care about
        for (int tau = 0; tau <= maxLag; ++tau)
            acf[tau] = fftData[tau];

        // 6. Compute NSDF normalization using prefix sums of squared samples
        //    m'(tau) = 2 * sum_{j=0}^{W-1-tau} (x[j]^2 + x[j+tau]^2)
        //    which equals 2 * (cumSum[W] - cumSum[tau] + cumSum[W+tau] - cumSum[tau])
        //    Simplified: use running sum approach
        float squaredSum[kWindowSize + kMaxLag + 1] = {};
        for (int i = 0; i < W + maxLag; ++i)
        {
            float s = readBuffer (i);
            squaredSum[i + 1] = squaredSum[i] + s * s;
        }

        // NSDF values
        float nsdf[kMaxLag + 1] = {};
        for (int tau = minLag - 1; tau <= maxLag; ++tau)
        {
            // m'(tau) = sum_{j=0}^{W-1} x[j]^2 + sum_{j=tau}^{W-1+tau} x[j]^2
            //         = (cumSum[W] - cumSum[0]) + (cumSum[W+tau] - cumSum[tau])
            float energy = (squaredSum[W] - squaredSum[0])
                         + (squaredSum[W + tau] - squaredSum[tau]);
            nsdf[tau] = (energy > 1e-12f) ? (2.0f * acf[tau] / energy) : 0.0f;
        }

        // 7. Peak picking using zero-crossing-based selection (McLeod method)
        //    Find positive regions after zero crossings, pick the highest peak
        //    in each region. Select the first peak above the threshold.
        struct Peak { int lag; float value; };
        Peak peaks[64];
        int numPeaks = 0;

        bool inPositiveRegion = (nsdf[minLag - 1] > 0.0f);
        // If NSDF is already positive AND decreasing at minLag, it's a boundary
        // decay artifact (dead note), not a rising pitch peak.
        bool skipFirstRegion = inPositiveRegion && (nsdf[minLag - 1] >= nsdf[minLag]);
        int  regionBestLag = inPositiveRegion ? (minLag - 1) : minLag;
        float regionBestVal = inPositiveRegion ? nsdf[minLag - 1] : -1.0f;

        for (int tau = minLag; tau <= maxLag; ++tau)
        {
            if (nsdf[tau] > 0.0f)
            {
                if (! inPositiveRegion)
                {
                    // Entering a new positive region
                    inPositiveRegion = true;
                    regionBestLag = tau;
                    regionBestVal = nsdf[tau];
                }
                else if (nsdf[tau] > regionBestVal)
                {
                    regionBestLag = tau;
                    regionBestVal = nsdf[tau];
                }
            }
            else if (inPositiveRegion)
            {
                // Exiting positive region — record the peak
                if (numPeaks < 64 && !skipFirstRegion)
                {
                    peaks[numPeaks].lag = regionBestLag;
                    peaks[numPeaks].value = regionBestVal;
                    ++numPeaks;
                }
                skipFirstRegion = false;   // clear after first region exits
                inPositiveRegion = false;
                regionBestVal = -1.0f;
            }
        }
        // Capture final region if still positive at maxLag
        if (inPositiveRegion && numPeaks < 64 && !skipFirstRegion)
        {
            peaks[numPeaks].lag = regionBestLag;
            peaks[numPeaks].value = regionBestVal;
            ++numPeaks;
        }

        if (numPeaks == 0)
        {
            detectedFrequency = 0.0f;
            return;
        }

        // Find global maximum peak value
        float maxPeakVal = -1.0f;
        for (int i = 0; i < numPeaks; ++i)
            if (peaks[i].value > maxPeakVal)
                maxPeakVal = peaks[i].value;

        if (maxPeakVal < 0.45f)
        {
            detectedFrequency = 0.0f;
            return; // confidence too low
        }

        // Select the first peak above 90% of the maximum
        const float threshold = maxPeakVal * 0.9f;
        int bestLag = -1;
        for (int i = 0; i < numPeaks; ++i)
        {
            if (peaks[i].value >= threshold)
            {
                bestLag = peaks[i].lag;
                break;
            }
        }

        if (bestLag < minLag)
            return;

        // 8. Parabolic interpolation for sub-sample accuracy
        float refinedLag = (float) bestLag;
        if (bestLag > minLag && bestLag < maxLag)
        {
            float a = nsdf[bestLag - 1];
            float b = nsdf[bestLag];
            float c = nsdf[bestLag + 1];
            float denom = 2.0f * (2.0f * b - a - c);
            if (std::abs (denom) > 1e-12f)
                refinedLag = bestLag + (a - c) / denom;
        }

        if (refinedLag > 0.0f)
        {
            float newFreq = (float) (sampleRate / refinedLag);

            // Frequency hysteresis: only update if the change exceeds 1%
            // to reduce jitter from frame-to-frame noise
            if (detectedFrequency <= 0.0f
                || std::abs (newFreq - detectedFrequency) / detectedFrequency > 0.01f)
            {
                detectedFrequency = newFreq;
            }
        }
    }
};

//==============================================================================
class BassSynthAudioProcessor : public juce::AudioProcessor
{
public:
    BassSynthAudioProcessor();
    ~BassSynthAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock; // expose the double-precision overload

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& dest) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    AutocorrelationPitchDetector detector;
    SawtoothOscillator oscillator;


    // Envelope follower for the noise gate
    float envelope = 0.0f;
    static constexpr float kEnvAttack  = 0.001f;
    static constexpr float kEnvRelease = 0.005f;
    static constexpr float kGateThreshold = 0.01f; // ~-40 dBFS

    double currentSampleRate = 48000;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BassSynthAudioProcessor)
};
