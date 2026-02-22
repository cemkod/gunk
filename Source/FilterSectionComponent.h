#pragma once

#include <JuceHeader.h>

class FilterSectionComponent : public juce::Component
{
public:
    explicit FilterSectionComponent (juce::AudioProcessorValueTreeState& apvts);
    ~FilterSectionComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void updateButtonStates();

private:
    juce::AudioProcessorValueTreeState& apvts;

    juce::Slider sensitivitySlider, resonanceSlider, decaySlider;
    juce::Label  sensitivityLabel,  resonanceLabel,  decayLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment sensitivityAttach, resonanceAttach, decayAttach;

    juce::TextButton sweepBtnOff  { "Off"  };
    juce::TextButton sweepBtnUp   { "Up"   };
    juce::TextButton sweepBtnDown { "Down" };

    void setSweepParam (int idx);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterSectionComponent)
};
