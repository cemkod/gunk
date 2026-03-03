#pragma once
#include "Filter.h"
#include <JuceHeader.h>
#include <cmath>
#include <atomic>

class EnvelopeFilter
{
public:
    // Call in prepareToPlay and also once per block (updates decay coeff)
    void prepare (double sampleRate, float decay)
    {
        sr = sampleRate;
        decayCoeff = 1.0f - std::exp (-1.0f / (float) (sr * decay));
    }

    void reset()
    {
        svf.reset();
    }

    // Returns filtered oscillator sample; also updates currentCutoffHz for UI.
    // filterFreq already includes any mod-matrix offset (Envelope/Pitch → Filter Freq).
    float processSample (float oscillatorSample,
                         float lastDetectedFreq, float filterFreq,
                         float freqTracking, float resonance)
    {
        static constexpr float kFilterCutoffMinHz = 20.0f;
        static constexpr float kFilterCutoffMaxHz = 4000.0f;
        const float cutoff = juce::jlimit (kFilterCutoffMinHz, kFilterCutoffMaxHz,
            filterFreq + lastDetectedFreq * freqTracking);

        currentCutoffHz.store (cutoff, std::memory_order_relaxed);
        return svf.process (oscillatorSample, cutoff, resonance, (float) sr);
    }

    float getCurrentCutoffHz() const { return currentCutoffHz.load (std::memory_order_relaxed); }

private:
    ResonantLowpassFilter svf;
    float decayCoeff = 0.0f;
    double sr = 48000.0;
    static constexpr float kDefaultCutoffHz = 200.0f;
    std::atomic<float> currentCutoffHz { kDefaultCutoffHz };
};
