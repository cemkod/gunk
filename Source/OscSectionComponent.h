#pragma once

#include <JuceHeader.h>
#include "LookAndFeel.h"
#include <functional>

struct OscParamIds
{
    juce::String waveform, level, unisonVoices, unisonDetune, unisonBlend, octaveShift, morph,
                 coarseTune, fineTune, unisonWidth;
};

// One oscillator's knobs: LEVEL MORPH COARSE FINE and the unison VOICES DETUNE BLEND WIDTH.
// The Oscillators panel owns the displays and combos and places the two knob rows.
// It is transparent to the mouse except for its knobs.
class OscSectionComponent : public juce::Component
{
public:
    // Callback for morph enable/disable (returns oscillator frame count)
    std::function<int()> getNumFrames;

    OscSectionComponent (const OscParamIds& ids, juce::AudioProcessorValueTreeState& apvts);

    // Rows are in this component's coordinates.
    void layoutKnobs (juce::Rectangle<int> mainRow, juce::Rectangle<int> unisonRow);

    void updateButtonStates();

private:
    juce::Slider levelSlider, morphSlider, coarseTuneSlider, fineTuneSlider;
    juce::Label  levelLabel,  morphLabel,  coarseTuneLabel,  fineTuneLabel;
    juce::Slider unisonVoicesSlider, unisonDetuneSlider, unisonBlendSlider, unisonWidthSlider;
    juce::Label  unisonVoicesLabel,  unisonDetuneLabel,  unisonBlendLabel,  unisonWidthLabel;

    juce::AudioProcessorValueTreeState::SliderAttachment levelAttach, morphAttach, coarseTuneAttach, fineTuneAttach,
                                                         unisonVoicesAttach, unisonDetuneAttach,
                                                         unisonBlendAttach, unisonWidthAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OscSectionComponent)
};
