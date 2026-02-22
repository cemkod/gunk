#pragma once

#include <JuceHeader.h>

class GateSectionComponent : public juce::Component
{
public:
    explicit GateSectionComponent (juce::AudioProcessorValueTreeState& apvts);
    ~GateSectionComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::Slider gateThresholdSlider, gateHysteresisSlider, glideSlider;
    juce::Label  gateThresholdLabel,  gateHysteresisLabel,  glideLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment gateThresholdAttach, gateHysteresisAttach, glideAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GateSectionComponent)
};
