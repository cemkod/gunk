#pragma once

#include <JuceHeader.h>
#include "SectionPanel.h"
#include "GateDisplayComponent.h"

// TRACKING: the gate meter strip, gate threshold/hysteresis, transient sensitivity, glide
// and the pitch detector window.
class GateSectionComponent : public SectionPanel
{
public:
    explicit GateSectionComponent (juce::AudioProcessorValueTreeState& apvts);

    void setMeterValues (float envelope, bool gateOpen, bool transientFired = false);

private:
    void layoutBody (juce::Rectangle<int> body) override;

    GateDisplayComponent displayComponent;

    juce::Slider gateThresholdSlider, gateHysteresisSlider, glideSlider, transientSensSlider;
    juce::Label  gateThresholdLabel,  gateHysteresisLabel,  glideLabel,  transientSensLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment gateThresholdAttach, gateHysteresisAttach, glideAttach, transientSensAttach;

    juce::ComboBox pitchWindowCombo;
    juce::Label    pitchWindowLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> pitchWindowAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GateSectionComponent)
};
