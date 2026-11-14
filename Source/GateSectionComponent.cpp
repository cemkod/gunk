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
      transientSensAttach  (avts, "transientSens",    transientSensSlider)
{
    BassLookAndFeel::setupRotarySlider (gateThresholdSlider,  gateThresholdLabel,  "THRS",  *this);
    BassLookAndFeel::setupRotarySlider (gateHysteresisSlider, gateHysteresisLabel, "HYST",  *this);
    BassLookAndFeel::setupRotarySlider (glideSlider,          glideLabel,          "GLIDE", *this);
    BassLookAndFeel::setupRotarySlider (drySlider,            dryLabel,            "DRY",   *this);
    BassLookAndFeel::setupRotarySlider (transientSensSlider, transientSensLabel, "SENS",  *this);
    addAndMakeVisible (displayComponent);

    pitchWindowCombo.addItem ("4096 (B0)", 1);
    pitchWindowCombo.addItem ("2048 (fast)", 2);
    // Attach after the items exist, so the initial selection is applied.
    pitchWindowAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        avts, "pitchWindow", pitchWindowCombo);
    pitchWindowCombo.setColour (juce::ComboBox::backgroundColourId, BassLookAndFeel::surface);
    pitchWindowCombo.setColour (juce::ComboBox::textColourId,       BassLookAndFeel::text);
    pitchWindowCombo.setColour (juce::ComboBox::outlineColourId,    BassLookAndFeel::border);

    pitchWindowLabel.setText ("WINDOW", juce::dontSendNotification);
    pitchWindowLabel.setFont (BassLookAndFeel::uiFont (10.0f));
    pitchWindowLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (pitchWindowCombo);
    addAndMakeVisible (pitchWindowLabel);
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
    BassLookAndFeel::placeKnobRow (inner.removeFromTop (UIConst::knobCellH),
                                   { { &gateThresholdSlider,  &gateThresholdLabel },
                                     { &gateHysteresisSlider, &gateHysteresisLabel },
                                     { &glideSlider,          &glideLabel } });

    inner.removeFromTop (UIConst::knobGap);

    // Row 2: DRY, SENS, then WINDOW label over its combo
    auto row2 = inner.removeFromTop (UIConst::knobCellH);
    const int colW = row2.getWidth() / 3;
    BassLookAndFeel::placeKnob (drySlider,           dryLabel,           row2.removeFromLeft (colW));
    BassLookAndFeel::placeKnob (transientSensSlider, transientSensLabel, row2.removeFromLeft (colW));
    auto windowArea = row2.reduced (2, 0).withSizeKeepingCentre (row2.getWidth() - 4,
                                                                 UIConst::knobNameH + 2 + UIConst::buttonH);
    pitchWindowLabel.setBounds (windowArea.removeFromTop (UIConst::knobNameH));
    windowArea.removeFromTop (2);
    pitchWindowCombo.setBounds (windowArea);
}

void GateSectionComponent::setMeterValues (float envelope, bool gateOpen, bool transientFired)
{
    displayComponent.pushSample (envelope, gateOpen, transientFired);
}
