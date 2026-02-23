#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// Formatter/parser factory functions for AudioParameterFloat definitions.
namespace ParamFormatters
{
    // 0–1 ↔ "50 %"
    inline auto pctFmt()
    {
        return [] (float v, int) -> juce::String
            { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; };
    }
    inline auto pctParse (float lo, float hi)
    {
        return [lo, hi] (const juce::String& t) -> float
            { return juce::jlimit (lo, hi, t.retainCharacters ("0123456789.").getFloatValue() / 100.0f); };
    }

    // 0–scale ↔ "50 %"
    inline auto scaledPctFmt (float scale)
    {
        return [scale] (float v, int) -> juce::String
            { return juce::String (juce::roundToInt (v / scale * 100.0f)) + " %"; };
    }
    inline auto scaledPctParse (float lo, float hi)
    {
        return [lo, hi] (const juce::String& t) -> float
            { return juce::jlimit (lo, hi, t.retainCharacters ("0123456789.").getFloatValue() / 100.0f * hi); };
    }

    // time ↔ "300 ms" / "1.20 s"
    inline auto timeFmt()
    {
        return [] (float v, int) -> juce::String
        {
            if (v < 1.0f) return juce::String (juce::roundToInt (v * 1000.0f)) + " ms";
            return juce::String (v, 2) + " s";
        };
    }
    inline auto timeParse (float lo, float hi)
    {
        return [lo, hi] (const juce::String& t) -> float
        {
            const float num = t.retainCharacters ("0123456789.").getFloatValue();
            const bool isSeconds = t.containsIgnoreCase ("s") && ! t.containsIgnoreCase ("ms");
            return juce::jlimit (lo, hi, isSeconds ? num : num / 1000.0f);
        };
    }
    // Logarithmic (dB-scale) NormalisableRange for linear amplitude parameters,
    // e.g. gateThreshold where 0.001–0.04 should feel perceptually even.
    inline juce::NormalisableRange<float> logAmpRange (float lo, float hi)
    {
        return juce::NormalisableRange<float> (
            lo, hi,
            [] (float start, float end, float v) -> float  // normalized → linear amplitude
            {
                const float dBStart = 20.0f * std::log10 (start);
                const float dBEnd   = 20.0f * std::log10 (end);
                return std::pow (10.0f, (dBStart + v * (dBEnd - dBStart)) / 20.0f);
            },
            [] (float start, float end, float v) -> float  // linear amplitude → normalized
            {
                const float dBStart = 20.0f * std::log10 (start);
                const float dBEnd   = 20.0f * std::log10 (end);
                const float dB      = 20.0f * std::log10 (juce::jmax (v, 1e-10f));
                return juce::jlimit (0.0f, 1.0f, (dB - dBStart) / (dBEnd - dBStart));
            });
    }

    // Formatter/parser pair for logAmpRange parameters — displays as dB.
    inline auto logAmpFmt()
    {
        return [] (float v, int) -> juce::String
            { return juce::String (20.0f * std::log10 (juce::jmax (v, 1e-10f)), 1) + " dB"; };
    }
    inline auto logAmpParse (float lo, float hi)
    {
        return [lo, hi] (const juce::String& t) -> float
        {
            const float dB = t.retainCharacters ("0123456789.-").getFloatValue();
            return juce::jlimit (lo, hi, std::pow (10.0f, dB / 20.0f));
        };
    }
} // namespace ParamFormatters
