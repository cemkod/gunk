#pragma once

#include <JuceHeader.h>

class GateSectionComponent : public juce::Component
{
public:
    explicit GateSectionComponent (juce::AudioProcessorValueTreeState& apvts);
    ~GateSectionComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void setMeterValues (float envelope, bool gateOpen);

private:
    juce::AudioProcessorValueTreeState& apvts;

    juce::Slider gateThresholdSlider, gateHysteresisSlider, glideSlider;
    juce::Label  gateThresholdLabel,  gateHysteresisLabel,  glideLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment gateThresholdAttach, gateHysteresisAttach, glideAttach;

    static constexpr int kMeterH = 12;
    juce::Rectangle<int> meterBounds;
    float meterEnvelope = 0.0f;
    bool  meterGateOpen = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GateSectionComponent)
};
