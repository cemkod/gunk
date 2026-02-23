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
} // namespace ParamFormatters
