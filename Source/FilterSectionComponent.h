#pragma once

#include <JuceHeader.h>
#include "FilterDisplayComponent.h"
#include "PluginProcessor.h"
#include "LabelledSectionComponent.h"

class FilterSectionComponent : public LabelledSectionComponent
{
public:
    FilterSectionComponent (JQGunkAudioProcessor& proc,
                            juce::AudioProcessorValueTreeState& apvts);
    ~FilterSectionComponent() override;

    void resized() override;

    void repaintDisplay() { displayComponent.repaint(); }

private:
    JQGunkAudioProcessor& processor;
    juce::AudioProcessorValueTreeState& apvts;

    FilterDisplayComponent displayComponent;

    juce::ComboBox filterTypeCombo;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment filterTypeAttach;

    juce::Slider filterFreqSlider, resonanceSlider, freqTrackingSlider;
    juce::Label  filterFreqLabel,  resonanceLabel,  freqTrackingLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment filterFreqAttach, resonanceAttach, freqTrackingAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterSectionComponent)
};
