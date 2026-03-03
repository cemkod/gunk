#pragma once

#include <JuceHeader.h>
#include "LabelledSectionComponent.h"
#include "GateDisplayComponent.h"

class GateSectionComponent : public LabelledSectionComponent
{
public:
    explicit GateSectionComponent (juce::AudioProcessorValueTreeState& avts);
    ~GateSectionComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void setMeterValues (float envelope, bool gateOpen, bool transientFired = false);

private:
    juce::AudioProcessorValueTreeState& apvts;

    GateDisplayComponent displayComponent;

    juce::Slider gateThresholdSlider, gateHysteresisSlider, glideSlider, drySlider, transientSlopeSlider;
    juce::Label  gateThresholdLabel,  gateHysteresisLabel,  glideLabel,  dryLabel,  transientSlopeLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment gateThresholdAttach, gateHysteresisAttach, glideAttach, dryAttach, transientSlopeAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GateSectionComponent)
};
