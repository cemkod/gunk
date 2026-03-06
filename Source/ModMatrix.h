#pragma once

#include <JuceHeader.h>
#include <array>

enum class ModSource : int { None = 0, Envelope = 1, Pitch = 2, ModEnvelope = 3, LFO = 4 };

enum class ModTarget : int
{
    None = 1, Morph1 = 2, Morph2 = 3, FilterFreq = 4, FilterRes = 5,
    Osc1Level = 6, Osc2Level = 7, Unison1Detune = 8, SubLevel = 9,
    Glide = 10, Unison2Detune = 11, LfoRate = 12, MasterVolume = 13,
    Osc1FineTune = 14, Osc2FineTune = 15, LfoAmount = 16,
    Unison1Blend = 17, Unison2Blend = 18
};

class ModMatrix
{
public:
    inline static const juce::StringArray kSourceNames {
        "None", "Envelope", "Pitch", "Mod Env", "LFO"
    };
    inline static const juce::StringArray kTargetNames {
        "None", "Morph 1", "Morph 2", "Filter Freq",
        "Filter Res", "OSC 1 Level", "OSC 2 Level",
        "Unison 1 Detune", "Sub Level",
        "Glide", "Unison 2 Detune", "LFO Rate", "Master Volume",
        "OSC 1 Fine Tune", "OSC 2 Fine Tune", "LFO Amount",
        "Uni1 Blend", "Uni2 Blend"
    };

    static void addParameters (juce::AudioProcessorValueTreeState::ParameterLayout& layout)
    {
        for (int i = 0; i < 8; ++i)
        {
            const juce::String n (i);
            layout.add (std::make_unique<juce::AudioParameterChoice> (
                "modSlot" + n + "Source", "Mod Slot " + n + " Source", kSourceNames, 0));
            layout.add (std::make_unique<juce::AudioParameterChoice> (
                "modSlot" + n + "Target", "Mod Slot " + n + " Target", kTargetNames, 0));
            layout.add (std::make_unique<juce::AudioParameterFloat> (
                "modSlot" + n + "Amount", "Mod Slot " + n + " Amount",
                juce::NormalisableRange<float> (-3.0f, 3.0f, 0.001f), 0.0f));
        }
    }


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
            slots[(size_t) i].target = (int) apvts.getRawParameterValue ("modSlot" + n + "Target")->load() + 1;
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
        0.f,     // index 0 (unused — enum starts at 1)
        0.f,     // None          (= 1, never queried directly)
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
