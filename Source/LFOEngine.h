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
        phase = std::fmod (modulatedRate / sampleRate * (double) numSamples + phase, 1.0);

        const float raw    = computeShape (shape, (float) phase);
        const float scaled = raw * modulatedAmount;
        valueAtomic.store (scaled, std::memory_order_relaxed);
        return scaled;
    }

    // Fills out[0..n-1] with per-sample LFO values, advancing phase sample-by-sample.
    // Call after updateModulated() each block.
    void renderBlock (float* out, int n, float shape, double sampleRate)
    {
        const double phaseInc = modulatedRate / sampleRate;
        for (int i = 0; i < n; ++i)
        {
            const float raw = computeShape (shape, (float) phase);
            out[i] = juce::jlimit (0.0f, 1.0f, raw * modulatedAmount);
            phase  = std::fmod (phase + phaseInc, 1.0);
        }
        valueAtomic.store (out[n - 1], std::memory_order_relaxed);
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
    double phase = 0.0;
    std::atomic<float> valueAtomic { 0.0f };

    static float computeShape (float shape, float p) noexcept
    {
        switch ((int) shape)
        {
            case 1:  return p < 0.5f ? p * 2.0f : 2.0f - p * 2.0f;  // Triangle
            case 2:  return p < 0.5f ? 1.0f : 0.0f;                   // Square
            case 3:  return p;                                          // Sawtooth
            default: return 0.5f + 0.5f * std::sin (juce::MathConstants<float>::twoPi * p); // Sine
        }
    }
};
