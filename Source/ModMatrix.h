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
        "None", "OSC 1 Morph", "OSC 2 Morph", "Filter Freq",
        "Filter Res", "OSC 1 Level", "OSC 2 Level",
        "OSC 1 Unison Detune", "Sub Level",
        "Glide", "OSC2 Unison Detune", "LFO Rate", "Master Volume",
        "OSC 1 Fine Tune", "OSC 2 Fine Tune", "LFO Amount",
        "OSC 1 Unison Blend", "OSC 2 Unison Blend"
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


    // Call once after the APVTS is constructed (e.g. in PluginProcessor constructor).
    // Caches raw parameter pointers so snapshot() never touches the heap.
    void init (juce::AudioProcessorValueTreeState& apvts)
    {
        for (int i = 0; i < 8; ++i)
        {
            juce::String n (i);
            slotPtrs[(size_t) i].source = apvts.getRawParameterValue ("modSlot" + n + "Source");
            slotPtrs[(size_t) i].target = apvts.getRawParameterValue ("modSlot" + n + "Target");
            slotPtrs[(size_t) i].amount = apvts.getRawParameterValue ("modSlot" + n + "Amount");
        }
    }

    // Called once per block before updateOscillatorParams().
    // envelopeVal: current amplitude follower value (0..1 range typical).
    // pitchHz:     last detected frequency in Hz (0 = no detection).
    void snapshot (float envelopeVal, float modEnvelopeVal, float pitchHz, float lfoVal = 0.0f)
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
            slots[(size_t) i].source = (int) slotPtrs[(size_t) i].source->load();
            slots[(size_t) i].target = (int) slotPtrs[(size_t) i].target->load() + 1;
            slots[(size_t) i].amount = slotPtrs[(size_t) i].amount->load();
        }
    }

    // Computes per-sample modulation contribution for `target` into outBuf.
    // Source buffers: envBuf=Envelope, pitchBuf=Pitch (normalized), modEnvBuf=ModEnv, lfoBuf=LFO.
    void computeTargetBlock (ModTarget target,
                             const float* envBuf, const float* modEnvBuf,
                             const float* pitchBuf, const float* lfoBuf,
                             float* outBuf, int n) const
    {
        juce::FloatVectorOperations::fill (outBuf, 0.0f, n);
        const int t = (int) target;
        if (t <= 0 || t >= (int) std::size (kTargetScale)) return;
        const float scale = kTargetScale[t];
        for (const auto& slot : slots)
        {
            if (slot.target != t || slot.source == 0 || slot.amount == 0.0f)
                continue;
            const float* src = (slot.source == 1) ? envBuf
                             : (slot.source == 2) ? pitchBuf
                             : (slot.source == 3) ? modEnvBuf
                             : (slot.source == 4) ? lfoBuf : nullptr;
            if (src)
                juce::FloatVectorOperations::addWithMultiply (outBuf, src, slot.amount * scale, n);
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

    struct SlotPtrs { std::atomic<float>* source = nullptr; std::atomic<float>* target = nullptr; std::atomic<float>* amount = nullptr; };
    std::array<SlotPtrs, 8> slotPtrs {};
};
