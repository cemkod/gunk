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
        attackCoeff = 1.0f - std::exp (-1.0f / (float) (sr * kAttack));
        decayCoeff  = 1.0f - std::exp (-1.0f / (float) (sr * decay));
    }

    void reset()
    {
        svf.reset();
        filterEnvelope = 0.0f;
    }

    // Returns filtered oscillator sample; also updates currentCutoffHz for UI
    float processSample (float inputSample, float oscillatorSample,
                         float lastDetectedFreq, float filterFreq,
                         float freqTracking, float sensitivity,
                         float resonance, int sweepMode)
    {
        // Envelope follower
        const float abs = std::abs (inputSample);
        if (abs > filterEnvelope)
            filterEnvelope += attackCoeff * (abs - filterEnvelope);
        else
            filterEnvelope += decayCoeff  * (abs - filterEnvelope);

        static constexpr float kFilterCutoffMinHz = 20.0f;
        static constexpr float kFilterCutoffMaxHz = 4000.0f;
        // Base cutoff with pitch tracking: knob freq + weighted detected pitch
        const float baseHz = juce::jlimit (kFilterCutoffMinHz, kFilterCutoffMaxHz,
            filterFreq + lastDetectedFreq * freqTracking);

        // Sweep
        float cutoff;
        if (sweepMode != 0)
        {
            float envControlled = juce::jlimit (0.0f, 1.0f, filterEnvelope * sensitivity);
            float sweepParam = (sweepMode == 2) ? (1.0f - envControlled) : envControlled;
            cutoff = baseHz * std::pow (kFilterCutoffMaxHz / baseHz, sweepParam);
        }
        else
        {
            cutoff = baseHz;
        }

        currentCutoffHz.store (cutoff, std::memory_order_relaxed);
        return svf.process (oscillatorSample, cutoff, resonance, (float) sr);
    }

    float getCurrentCutoffHz() const { return currentCutoffHz.load (std::memory_order_relaxed); }

private:
    ResonantLowpassFilter svf;
    float filterEnvelope = 0.0f;
    float attackCoeff = 0.0f;
    float decayCoeff  = 0.0f;
    double sr = 48000.0;
    static constexpr float kDefaultCutoffHz = 200.0f;
    std::atomic<float> currentCutoffHz { kDefaultCutoffHz };
    static constexpr float kAttack = 0.001f;
};
