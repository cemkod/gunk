#pragma once
#include <JuceHeader.h>

// Self-contained LFO: phase accumulator, shape, rate/amount modulation.
struct LFOEngine
{
    // Advance phase and compute output for this block.
    // Uses modulatedRate/modulatedAmount from the previous block so that
    // LfoRate/LfoAmount mod targets take effect without a one-block lag.
    // Returns the scaled LFO value [0, 1].
    float tick (float shape, int numSamples, double sampleRate)
    {
        phase = std::fmod (modulatedRate / (float) sampleRate * (float) numSamples + phase, 1.0f);

        float raw;
        switch ((int) shape)
        {
            case 1:  raw = phase < 0.5f ? phase * 2.0f : 2.0f - phase * 2.0f; break; // Triangle
            case 2:  raw = phase < 0.5f ? 1.0f : 0.0f; break;                         // Square
            case 3:  raw = phase; break;                                                // Sawtooth
            default: raw = 0.5f + 0.5f * std::sin (juce::MathConstants<float>::twoPi * phase); // Sine
        }

        const float scaled = raw * modulatedAmount;
        valueAtomic.store (scaled, std::memory_order_relaxed);
        return scaled;
    }

    // Call after modMatrix.snapshot() to update rate/amount for the next block.
    void updateModulated (float baseRate,   float rateOffset,
                          float baseAmount, float amountOffset)
    {
        modulatedRate   = juce::jlimit (0.01f, 20.0f, baseRate   + rateOffset);
        modulatedAmount = juce::jlimit (0.0f,  1.0f,  baseAmount + amountOffset);
    }

    // Thread-safe read for the UI.
    float getValue() const { return valueAtomic.load (std::memory_order_relaxed); }

    // Exposed so BlockParams / mod targets can read the current effective values.
    float modulatedRate   = 1.0f;
    float modulatedAmount = 1.0f;

private:
    float phase = 0.0f;
    std::atomic<float> valueAtomic { 0.0f };
};
