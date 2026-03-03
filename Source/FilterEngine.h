#pragma once
#include "Filter.h"
#include <JuceHeader.h>
#include <cmath>
#include <atomic>

class EnvelopeFilter
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
    }

    void reset()
    {
        svf.reset();
    }

    // Returns filtered oscillator sample; also updates currentCutoffHz for UI.
    // filterFreq already includes any mod-matrix offset (Envelope/Pitch → Filter Freq).
    float processSample (float oscillatorSample,
                         float lastDetectedFreq, float filterFreq,
                         float freqTracking, float resonance,
                         FilterType filterType = FilterType::LP)
    {
        static constexpr float kFilterCutoffMinHz = 20.0f;
        static constexpr float kFilterCutoffMaxHz = 4000.0f;
        const float cutoff = juce::jlimit (kFilterCutoffMinHz, kFilterCutoffMaxHz,
            filterFreq + lastDetectedFreq * freqTracking);

        currentCutoffHz.store (cutoff, std::memory_order_relaxed);
        return svf.process (oscillatorSample, cutoff, resonance, (float) sr, filterType);
    }

    float getCurrentCutoffHz() const { return currentCutoffHz.load (std::memory_order_relaxed); }

private:
    StateVariableFilter svf;
    double sr = 48000.0;
    static constexpr float kDefaultCutoffHz = 200.0f;
    std::atomic<float> currentCutoffHz { kDefaultCutoffHz };
};
