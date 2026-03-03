#pragma once

#include <JuceHeader.h>
#include <cmath>

//==============================================================================
enum class FilterType { LP = 0, HP, BP };

struct StateVariableFilter
{
    float low = 0.0f, band = 0.0f;

    float process (float input, float cutoffHz, float Q, float sampleRate,
                   FilterType filterType = FilterType::LP)
    {
        static constexpr float kSVFMaxF = 1.99f;
        float f = 2.0f * std::sin (juce::MathConstants<float>::pi * cutoffHz / sampleRate);
        f = std::min (f, kSVFMaxF);
        // q (damping) must satisfy q < 2/f for SVF stability; also clamp Q >= 0.5 (overdamped)
        float q = 1.0f / std::max (Q, 0.5f);
        q = std::min (q, 2.0f / f - 0.01f);
        low  += f * band;
        float high = input - low - q * band;
        band += f * high;

        switch (filterType)
        {
            case FilterType::HP: return high;
            case FilterType::BP: return band;
            case FilterType::LP:
            default:             return low;
        }
    }

    void reset() { low = band = 0.0f; }
};
