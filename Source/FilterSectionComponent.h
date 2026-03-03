#pragma once

#include <JuceHeader.h>
#include "FilterDisplayComponent.h"
#include "PluginProcessor.h"
#include "LabelledSectionComponent.h"

class FilterSectionComponent : public LabelledSectionComponent,
                                public juce::AudioProcessorValueTreeState::Listener
{
public:
    FilterSectionComponent (JQGunkAudioProcessor& proc,
                            juce::AudioProcessorValueTreeState& apvts);
    ~FilterSectionComponent() override;

    void resized() override;
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    void repaintDisplay() { displayComponent.repaint(); }

private:
    void updateButtonStates (int filterTypeIdx);

    JQGunkAudioProcessor& processor;
    juce::AudioProcessorValueTreeState& apvts;

    FilterDisplayComponent displayComponent;

    juce::TextButton lpButton { "LP" }, hpButton { "HP" }, bpButton { "BP" };

    juce::Slider filterFreqSlider, resonanceSlider, freqTrackingSlider;
    juce::Label  filterFreqLabel,  resonanceLabel,  freqTrackingLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment filterFreqAttach, resonanceAttach, freqTrackingAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterSectionComponent)
};
