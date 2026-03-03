#pragma once

#include <JuceHeader.h>
#include "LabelledSectionComponent.h"
#include <functional>

struct OscParamIds
{
    juce::String waveform, level, unisonVoices, unisonDetune, unisonBlend, octaveShift, morph,
                 coarseTune, fineTune;
};

class OscSectionComponent : public LabelledSectionComponent
{
public:
    // Callback for morph enable/disable (returns oscillator frame count)
    std::function<int()> getNumFrames;

    OscSectionComponent (const juce::String& title,
                         const OscParamIds& ids,
                         juce::AudioProcessorValueTreeState& apvts,
                         bool embedded = false);
    ~OscSectionComponent() override;

    void resized() override;

    void updateButtonStates();

private:
    OscParamIds paramIds;
    juce::AudioProcessorValueTreeState& apvts;
    bool embedded_;

    juce::Slider levelSlider;
    juce::Label  levelLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment levelAttach;

    juce::Slider unisonVoicesSlider, unisonDetuneSlider, unisonBlendSlider;
    juce::Label  unisonVoicesLabel,  unisonDetuneLabel,  unisonBlendLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment unisonVoicesAttach,
                                                          unisonDetuneAttach,
                                                          unisonBlendAttach;

    juce::Slider morphSlider;
    juce::Label  morphLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment morphAttach;

    juce::Slider coarseTuneSlider, fineTuneSlider;
    juce::Label  coarseTuneLabel,  fineTuneLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment coarseTuneAttach, fineTuneAttach;

    juce::TextButton octBtn0 { "0" }, octBtn1 { "+1" }, octBtn2 { "+2" };
    juce::Label      octLabel;

    void configureOctaveButtons();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OscSectionComponent)
};
