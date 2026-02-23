#pragma once

#include <JuceHeader.h>
#include <cmath>

//==============================================================================
struct ResonantLowpassFilter
{
    float low = 0.0f, band = 0.0f;

    float process (float input, float cutoffHz, float Q, float sampleRate)
    {
        static constexpr float kSVFMaxF = 1.99f;
        static constexpr float kSVFMinQ = 0.01f;
        float f = 2.0f * std::sin (juce::MathConstants<float>::pi * cutoffHz / sampleRate);
        f = std::min (f, kSVFMaxF);
        float q = 1.0f / std::max (Q, kSVFMinQ);
        low  += f * band;
        float high = input - low - q * band;
        band += f * high;
        return low;
    }

    void reset() { low = band = 0.0f; }
};
