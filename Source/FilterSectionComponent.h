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

    juce::Slider filterFreqSlider, resonanceSlider, decaySlider, freqTrackingSlider;
    juce::Label  filterFreqLabel,  resonanceLabel,  decayLabel,  freqTrackingLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment filterFreqAttach, resonanceAttach, decayAttach, freqTrackingAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterSectionComponent)
};
