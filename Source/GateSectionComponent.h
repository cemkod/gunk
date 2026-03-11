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

    juce::Slider gateThresholdSlider, gateHysteresisSlider, glideSlider, drySlider, transientSensSlider;
    juce::Label  gateThresholdLabel,  gateHysteresisLabel,  glideLabel,  dryLabel,  transientSensLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment gateThresholdAttach, gateHysteresisAttach, glideAttach, dryAttach, transientSensAttach;

    juce::ComboBox pitchWindowCombo;
    juce::Label    pitchWindowLabel;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment pitchWindowAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GateSectionComponent)
};
