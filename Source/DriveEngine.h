#pragma once

#include <JuceHeader.h>
#include <cmath>

// Mono waveshaper between the oscillator mix and the filter.
// Runs at 2x through juce::dsp::Oversampling (polyphase IIR, low latency) so the
// shaper's harmonics above Nyquist are filtered before decimation.
class DriveEngine
{
public:
    enum class Curve : int { Soft = 0, Hard = 1, Tube = 2 };

    void prepare (double sampleRate, int maxBlockSize)
    {
        oversampler.initProcessing ((size_t) maxBlockSize);
        oversampler.reset();
        gain.reset (sampleRate * (double) oversampler.getOversamplingFactor(), 0.02);
        gain.setCurrentAndTargetValue (1.0f);
        dcCoef = 1.0f - (juce::MathConstants<float>::twoPi * 10.0f / (float) sampleRate);
        dcX1 = dcY1 = 0.0f;
        wasActive = false;
    }

    // In place. driveDb is the input gain into the shaper (0..36 dB).
    void processBlock (float* data, int n, float driveDb, Curve curve)
    {
        if (! wasActive)
        {
            // Coming out of bypass: drop stale filter state and jump to the target gain.
            oversampler.reset();
            gain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (driveDb));
            dcX1 = dcY1 = 0.0f;
            wasActive = true;
        }
        gain.setTargetValue (juce::Decibels::decibelsToGain (driveDb));

        float* channels[] = { data };
        juce::dsp::AudioBlock<float> block (channels, 1, (size_t) n);
        auto up = oversampler.processSamplesUp (block);
        float* u = up.getChannelPointer (0);
        const int upN = (int) up.getNumSamples();

        switch (curve)
        {
            case Curve::Soft: shape (u, upN, softClip); break;
            case Curve::Hard: shape (u, upN, hardClip); break;
            case Curve::Tube: shape (u, upN, tube);     break;
        }

        oversampler.processSamplesDown (block);

        // DC blocker: the tube curve is asymmetric and leaves an offset.
        for (int i = 0; i < n; ++i)
        {
            const float x = data[i];
            const float y = x - dcX1 + dcCoef * dcY1;
            dcX1 = x;
            dcY1 = y;
            data[i] = y;
        }
    }

    // Call on blocks where the drive section is off, so the next enable starts clean.
    void bypass() noexcept { wasActive = false; }

private:
    static float softClip (float x) noexcept { return std::tanh (x); }
    static float hardClip (float x) noexcept { return juce::jlimit (-1.0f, 1.0f, x); }
    // Positive half clips like tanh; negative half saturates later and softer,
    // which adds even harmonics.
    static float tube (float x) noexcept
    {
        return x >= 0.0f ? std::tanh (x) : 2.0f * std::tanh (0.5f * x);
    }

    // Auto-gain keeps a reference-level signal (peak kRef) within ~3 dB at any drive:
    // the output is scaled by kRef over the mean of the shaper's two half-peaks.
    // For the symmetric curves that is kRef / shaper(g * kRef).
    template <typename Fn>
    void shape (float* u, int n, Fn fn) noexcept
    {
        static constexpr float kRef = 0.5f;
        for (int i = 0; i < n; ++i)
        {
            const float g    = gain.getNextValue();
            const float comp = 2.0f * kRef / (fn (g * kRef) - fn (-g * kRef));
            u[i] = fn (g * u[i]) * comp;
        }
    }

    juce::dsp::Oversampling<float> oversampler {
        1, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false, false };
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> gain { 1.0f };
    float dcCoef = 0.999f, dcX1 = 0.0f, dcY1 = 0.0f;
    bool  wasActive = false;
};
