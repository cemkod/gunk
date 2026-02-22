#include "GateSectionComponent.h"
#include "LookAndFeel.h"

GateSectionComponent::GateSectionComponent (juce::AudioProcessorValueTreeState& apvts)
    : gateThresholdAttach  (apvts, "gateThreshold",  gateThresholdSlider),
      gateHysteresisAttach (apvts, "gateHysteresis", gateHysteresisSlider),
      glideAttach          (apvts, "glide",           glideSlider)
{
    auto setupSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& name)
    {
        s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
        s.setColour (juce::Slider::textBoxTextColourId,       BassLookAndFeel::text);
        s.setColour (juce::Slider::textBoxBackgroundColourId, BassLookAndFeel::surfaceDark);
        s.setColour (juce::Slider::textBoxOutlineColourId,    BassLookAndFeel::borderDim);
        addAndMakeVisible (s);

        l.setText (name, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.setFont (juce::Font (11.0f, juce::Font::bold));
        l.setColour (juce::Label::textColourId, BassLookAndFeel::text);
        addAndMakeVisible (l);
    };

    setupSlider (gateThresholdSlider,  gateThresholdLabel,  "THRS");
    setupSlider (gateHysteresisSlider, gateHysteresisLabel, "HYST");
    setupSlider (glideSlider,          glideLabel,          "GLIDE");
}

GateSectionComponent::~GateSectionComponent()
{
    setLookAndFeel (nullptr);
}

void GateSectionComponent::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds();
    g.setColour (BassLookAndFeel::surface);
    g.fillRoundedRectangle (bounds.toFloat(), 6.0f);
    g.setColour (BassLookAndFeel::border);
    g.drawRoundedRectangle (bounds.toFloat(), 6.0f, 1.0f);
    g.setColour (BassLookAndFeel::text);
    g.setFont (juce::Font (11.0f, juce::Font::bold));
    g.drawText ("GATE/TRACKING", bounds.reduced (6, 4).removeFromTop (14), juce::Justification::topLeft);
}

void GateSectionComponent::resized()
{
    auto inner = getLocalBounds().reduced (8);
    inner.removeFromTop (18); // skip section label row
    inner.removeFromTop (10);
    const int knobW = inner.getWidth() / 3;
    auto knobRow = inner.removeFromTop (75);
    gateThresholdSlider .setBounds (knobRow.removeFromLeft (knobW));
    gateHysteresisSlider.setBounds (knobRow.removeFromLeft (knobW));
    glideSlider         .setBounds (knobRow.removeFromLeft (knobW));
    auto lblRow = inner.removeFromTop (18);
    gateThresholdLabel .setBounds (lblRow.removeFromLeft (knobW));
    gateHysteresisLabel.setBounds (lblRow.removeFromLeft (knobW));
    glideLabel         .setBounds (lblRow.removeFromLeft (knobW));
}
