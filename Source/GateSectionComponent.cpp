#include "GateSectionComponent.h"
#include "LookAndFeel.h"

GateSectionComponent::GateSectionComponent (juce::AudioProcessorValueTreeState& avts)
    : SectionPanel ("TRACKING"),
      displayComponent (avts),
      gateThresholdAttach  (avts, "gateThreshold",  gateThresholdSlider),
      gateHysteresisAttach (avts, "gateHysteresis", gateHysteresisSlider),
      glideAttach          (avts, "glide",          glideSlider),
      transientSensAttach  (avts, "transientSens",  transientSensSlider)
{
    // The meter strip is the only feedback for setting the threshold
    addAndMakeVisible (displayComponent);

    BassLookAndFeel::setupRotarySlider (gateThresholdSlider,  gateThresholdLabel,  "THRESHOLD",   *this);
    BassLookAndFeel::setupRotarySlider (gateHysteresisSlider, gateHysteresisLabel, "HYSTERESIS",  *this);
    BassLookAndFeel::setupRotarySlider (transientSensSlider,  transientSensLabel,  "SENSITIVITY", *this);
    BassLookAndFeel::setupRotarySlider (glideSlider,          glideLabel,          "GLIDE",       *this);

    pitchWindowCombo.addItem ("4096 (B0)", 1);
    pitchWindowCombo.addItem ("2048 (fast)", 2);
    // Attach after the items exist, so the initial selection is applied.
    pitchWindowAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        avts, "pitchWindow", pitchWindowCombo);
    pitchWindowCombo.setTooltip ("Pitch detector window: 4096 tracks down to B0, 2048 responds faster");
    addAndMakeVisible (pitchWindowCombo);
    BassLookAndFeel::setupCaption (pitchWindowLabel, "WINDOW", *this);
}

void GateSectionComponent::layoutBody (juce::Rectangle<int> body)
{
    displayComponent.setBounds (body.removeFromTop (20));
    body.removeFromTop (4);

    auto windowRow = body.removeFromBottom (UIConst::buttonH);
    pitchWindowLabel.setBounds (windowRow.removeFromLeft (64));
    pitchWindowCombo.setBounds (windowRow);
    body.removeFromBottom (4);

    // 2 x 2 knobs: THRESHOLD HYSTERESIS / SENSITIVITY GLIDE (small)
    const int colW = body.getWidth() / 2;
    auto row1 = body.removeFromTop (body.getHeight() / 2);
    auto row2 = body;
    BassLookAndFeel::placeKnob (gateThresholdSlider,  gateThresholdLabel,  row1.removeFromLeft (colW));
    BassLookAndFeel::placeKnob (gateHysteresisSlider, gateHysteresisLabel, row1);
    BassLookAndFeel::placeKnob (transientSensSlider,  transientSensLabel,  row2.removeFromLeft (colW));
    BassLookAndFeel::placeKnob (glideSlider,          glideLabel,          row2, BassLookAndFeel::KnobSize::small);
}

void GateSectionComponent::setMeterValues (float envelope, bool gateOpen, bool transientFired)
{
    displayComponent.pushSample (envelope, gateOpen, transientFired);
}
