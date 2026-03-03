#include "GateSectionComponent.h"
#include "LookAndFeel.h"


GateSectionComponent::GateSectionComponent (juce::AudioProcessorValueTreeState& avts)
    : LabelledSectionComponent ("GATE/TRACKING"),
      apvts (avts),
      displayComponent (avts),
      gateThresholdAttach  (avts, "gateThreshold",   gateThresholdSlider),
      gateHysteresisAttach (avts, "gateHysteresis",  gateHysteresisSlider),
      glideAttach          (avts, "glide",            glideSlider),
      dryAttach            (avts, "dryLevel",         drySlider),
      transientSlopeAttach (avts, "transientSlope",   transientSlopeSlider)
{
    BassLookAndFeel::setupRotarySlider (gateThresholdSlider,  gateThresholdLabel,  "THRS",  *this);
    BassLookAndFeel::setupRotarySlider (gateHysteresisSlider, gateHysteresisLabel, "HYST",  *this);
    BassLookAndFeel::setupRotarySlider (glideSlider,          glideLabel,          "GLIDE", *this);
    BassLookAndFeel::setupRotarySlider (drySlider,            dryLabel,            "DRY",   *this);
    BassLookAndFeel::setupRotarySlider (transientSlopeSlider, transientSlopeLabel, "SLOPE", *this);
    addAndMakeVisible (displayComponent);
}

GateSectionComponent::~GateSectionComponent()
{
    setLookAndFeel (nullptr);
}

void GateSectionComponent::paint (juce::Graphics& g)
{
    LabelledSectionComponent::paint (g);
}

void GateSectionComponent::resized()
{
    auto inner = getLocalBounds().reduced (8);
    inner.removeFromTop (18); // skip section label row
    displayComponent.setBounds (inner.removeFromTop (70));
    inner.removeFromTop (8);  // gap between graph and knobs
    const int knobW = inner.getWidth() / 5;
    auto knobRow = inner.removeFromTop (75);
    gateThresholdSlider .setBounds (knobRow.removeFromLeft (knobW));
    gateHysteresisSlider.setBounds (knobRow.removeFromLeft (knobW));
    glideSlider         .setBounds (knobRow.removeFromLeft (knobW));
    drySlider           .setBounds (knobRow.removeFromLeft (knobW));
    transientSlopeSlider.setBounds (knobRow);
    auto lblRow = inner.removeFromTop (18);
    gateThresholdLabel .setBounds (lblRow.removeFromLeft (knobW));
    gateHysteresisLabel.setBounds (lblRow.removeFromLeft (knobW));
    glideLabel         .setBounds (lblRow.removeFromLeft (knobW));
    dryLabel           .setBounds (lblRow.removeFromLeft (knobW));
    transientSlopeLabel.setBounds (lblRow);
}

void GateSectionComponent::setMeterValues (float envelope, bool gateOpen, bool transientFired)
{
    displayComponent.pushSample (envelope, gateOpen, transientFired);
}
