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
    auto inner = getLocalBounds().reduced (UIConst::sectionInnerPad);
    inner.removeFromTop (UIConst::sectionHeaderH); // skip section label row
    displayComponent.setBounds (inner.removeFromTop (UIConst::displayH_small));
    inner.removeFromTop (UIConst::knobGap);

    // Row 1: THRS, HYST, GLIDE
    const int w = inner.getWidth();
    const int knobW1 = w / 3;
    auto knobRow1 = inner.removeFromTop (UIConst::knobRowH);
    gateThresholdSlider .setBounds (knobRow1.removeFromLeft (knobW1));
    gateHysteresisSlider.setBounds (knobRow1.removeFromLeft (knobW1));
    glideSlider         .setBounds (knobRow1);
    auto lblRow1 = inner.removeFromTop (UIConst::knobLabelH);
    gateThresholdLabel .setBounds (lblRow1.removeFromLeft (knobW1));
    gateHysteresisLabel.setBounds (lblRow1.removeFromLeft (knobW1));
    glideLabel         .setBounds (lblRow1);

    inner.removeFromTop (UIConst::knobGap);

    // Row 2: DRY, SLOPE
    const int knobW2 = w / 2;
    auto knobRow2 = inner.removeFromTop (UIConst::knobRowH);
    drySlider           .setBounds (knobRow2.removeFromLeft (knobW2));
    transientSlopeSlider.setBounds (knobRow2);
    auto lblRow2 = inner.removeFromTop (UIConst::knobLabelH);
    dryLabel           .setBounds (lblRow2.removeFromLeft (knobW2));
    transientSlopeLabel.setBounds (lblRow2);
}

void GateSectionComponent::setMeterValues (float envelope, bool gateOpen, bool transientFired)
{
    displayComponent.pushSample (envelope, gateOpen, transientFired);
}
