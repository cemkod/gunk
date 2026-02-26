#pragma once

#include <JuceHeader.h>
#include "LabelledSectionComponent.h"

class GateSectionComponent : public LabelledSectionComponent
{
public:
    explicit GateSectionComponent (juce::AudioProcessorValueTreeState& avts);
    ~GateSectionComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void setMeterValues (float envelope, bool gateOpen);

private:
    juce::AudioProcessorValueTreeState& apvts;

    juce::Slider gateThresholdSlider, gateHysteresisSlider, glideSlider, drySlider, transientSlopeSlider;
    juce::Label  gateThresholdLabel,  gateHysteresisLabel,  glideLabel,  dryLabel,  transientSlopeLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment gateThresholdAttach, gateHysteresisAttach, glideAttach, dryAttach, transientSlopeAttach;

    static constexpr int kMeterH = 12;
    juce::Rectangle<int> meterBounds;
    float meterEnvelope = 0.0f;
    bool  meterGateOpen = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GateSectionComponent)
};
