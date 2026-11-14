#pragma once

#include <JuceHeader.h>
#include <cmath>

// 12 dB/oct Butterworth high-pass (TPT state-variable form) for the side signal.
// Removing the side signal's low end keeps everything below the cutoff mono.
class SideHighpass
{
public:
    void prepare (double newSampleRate) { sampleRate = newSampleRate; reset(); }
    void reset() noexcept               { ic1 = ic2 = 0.0f; }

    // In place. Coefficients are block-rate; the cutoff is a plain parameter.
    void processBlock (float* buf, int n, float cutoffHz) noexcept
    {
        const float fc = juce::jlimit (10.0f, (float) (0.45 * sampleRate), cutoffHz);
        const float g  = std::tan (juce::MathConstants<float>::pi * fc / (float) sampleRate);
        const float k  = juce::MathConstants<float>::sqrt2;   // Q = 1/sqrt(2)
        const float a1 = 1.0f / (1.0f + g * (g + k));
        const float a2 = g * a1;
        const float a3 = g * a2;

        for (int i = 0; i < n; ++i)
        {
            const float x  = buf[i];
            const float v3 = x - ic2;
            const float v1 = a1 * ic1 + a2 * v3;
            const float v2 = ic2 + a2 * ic1 + a3 * v3;
            ic1 = 2.0f * v1 - ic1;
            ic2 = 2.0f * v2 - ic2;
            buf[i] = x - k * v1 - v2;
        }
    }

private:
    double sampleRate = 48000.0;
    float  ic1 = 0.0f, ic2 = 0.0f;
};
