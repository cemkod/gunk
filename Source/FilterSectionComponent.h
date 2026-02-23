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

    void updateButtonStates();

    void repaintDisplay() { displayComponent.repaint(); }

private:
    JQGunkAudioProcessor& processor;
    juce::AudioProcessorValueTreeState& apvts;

    FilterDisplayComponent displayComponent;

    juce::Slider filterFreqSlider, sensitivitySlider, resonanceSlider, decaySlider, freqTrackingSlider;
    juce::Label  filterFreqLabel,  sensitivityLabel,  resonanceLabel,  decayLabel,  freqTrackingLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment filterFreqAttach, sensitivityAttach, resonanceAttach, decayAttach, freqTrackingAttach;

    juce::TextButton sweepBtnOff  { "Off"  };
    juce::TextButton sweepBtnUp   { "Up"   };
    juce::TextButton sweepBtnDown { "Down" };

    void setSweepParam (int idx);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterSectionComponent)
};
