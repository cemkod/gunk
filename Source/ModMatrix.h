#pragma once

#include <JuceHeader.h>
#include <array>

enum class ModSource : int { None = 0, Envelope = 1, Pitch = 2, ModEnvelope = 3, LFO = 4 };

enum class ModTarget : int
{
    None = 0, Morph1 = 1, Morph2 = 2, FilterFreq = 3, FilterRes = 4,
    Osc1Level = 5, Osc2Level = 6, Unison1Detune = 7, SubLevel = 8,
    Glide = 9, Unison2Detune = 10, LfoRate = 11, MasterVolume = 12,
    Osc1FineTune = 13, Osc2FineTune = 14, LfoAmount = 15,
    Unison1Blend = 16, Unison2Blend = 17
};

class ModMatrix
{
public:
    // Called once per block before updateOscillatorParams().
    // envelopeVal: current amplitude follower value (0..1 range typical).
    // pitchHz:     last detected frequency in Hz (0 = no detection).
    void snapshot (juce::AudioProcessorValueTreeState& apvts,
                   float envelopeVal,
                   float modEnvelopeVal,
                   float pitchHz,
                   float lfoVal = 0.0f)
    {
        sourceVals[0] = 0.0f;  // None
        sourceVals[1] = envelopeVal;
        sourceVals[2] = (pitchHz > 0.0f)
                        ? juce::jlimit (0.0f, 1.0f, (pitchHz - 40.0f) / 360.0f)
                        : 0.0f;
        sourceVals[3] = modEnvelopeVal;
        sourceVals[4] = lfoVal;

        for (int i = 0; i < 8; ++i)
        {
            const juce::String n (i);
            slots[(size_t) i].source = (int) apvts.getRawParameterValue ("modSlot" + n + "Source")->load();
            slots[(size_t) i].target = (int) apvts.getRawParameterValue ("modSlot" + n + "Target")->load();
            slots[(size_t) i].amount = apvts.getRawParameterValue ("modSlot" + n + "Amount")->load();
        }
    }

    // Returns the summed modulation offset (in target parameter units) for a given target.
    float getOffset (ModTarget target) const
    {
        const int t = (int) target;
        if (t <= 0 || t >= (int) std::size (kTargetScale))
            return 0.0f;

        float sum = 0.0f;
        for (const auto& slot : slots)
        {
            if (slot.target != t) continue;
            if (slot.source < 0 || slot.source >= 5) continue;
            sum += slot.amount * sourceVals[slot.source] * kTargetScale[t];
        }
        return sum;
    }

private:
    // Scale: amount(-1..1) * sourceVal(0..1) * scale = offset in parameter units.
    static constexpr float kTargetScale[] = {
        0.f,     // None
        1.f,     // Morph1        (range 0..1)
        1.f,     // Morph2        (range 0..1)
        6000.f,  // FilterFreq    (range -2000..4000, span 6000)
        8.f,     // FilterRes     (range 0..8)
        2.f,     // Osc1Level     (range 0..2)
        2.f,     // Osc2Level     (range 0..2)
        100.f,   // Unison1Detune (range 0..100)
        1.f,     // SubLevel      (range 0..1)
        1.f,     // Glide         (range 0..1)
        100.f,   // Unison2Detune (range 0..100)
        20.f,    // LfoRate       (range 0.01..20)
        2.f,     // MasterVolume  (range 0..2)
        200.f,   // Osc1FineTune  (cents; amount=1,src=1 → 200 ct = 2 st)
        200.f,   // Osc2FineTune  (cents; amount=1,src=1 → 200 ct = 2 st)
        1.f,     // LfoAmount     (range 0..1)
        1.f,     // Unison1Blend  (range 0..1)
        1.f,     // Unison2Blend  (range 0..1)
    };

    struct Slot { int source = 0; int target = 0; float amount = 0.0f; };
    std::array<Slot, 8> slots {};
    float sourceVals[5] = {};
};
