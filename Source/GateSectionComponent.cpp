#include "GateSectionComponent.h"
#include "LookAndFeel.h"

GateSectionComponent::GateSectionComponent (juce::AudioProcessorValueTreeState& apvts)
    : apvts (apvts),
      gateThresholdAttach  (apvts, "gateThreshold",  gateThresholdSlider),
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

    // --- VU meter strip ---
    {
        auto mb = meterBounds.toFloat();

        // Background
        g.setColour (BassLookAndFeel::surfaceDark);
        g.fillRoundedRectangle (mb, 3.0f);

        // dB-scale helper: maps linear amplitude to 0..1 fill fraction (-60..0 dBFS)
        const float dBMin = -60.0f;
        auto toFill = [dBMin] (float linear) -> float {
            const float dB = 20.0f * std::log10 (juce::jmax (linear, 1e-10f));
            return juce::jlimit (0.0f, 1.0f, (dB - dBMin) / -dBMin);
        };

        // Level bar
        const float fillFrac = toFill (meterEnvelope);
        if (fillFrac > 0.0f)
        {
            const juce::Colour barColour = meterGateOpen
                ? juce::Colour (0xff44ff88)
                : juce::Colour (0xff44ff88).withAlpha (0.4f);
            g.setColour (barColour);
            g.fillRoundedRectangle (mb.withWidth (mb.getWidth() * fillFrac), 3.0f);
        }

        // Threshold marker lines (read live from APVTS)
        const float thresh = *apvts.getRawParameterValue ("gateThreshold");
        const float hyst   = *apvts.getRawParameterValue ("gateHysteresis");

        // Close threshold (orange-red)
        const float threshX = mb.getX() + toFill (thresh) * mb.getWidth();
        g.setColour (juce::Colour (0xffff6633).withAlpha (0.9f));
        g.drawVerticalLine (juce::roundToInt (threshX), mb.getY(), mb.getBottom());

        // Open threshold (amber, dimmer) — hyst is in dB
        const float openX = mb.getX() + toFill (thresh * std::pow (10.0f, hyst / 20.0f)) * mb.getWidth();
        if (openX > threshX + 1.0f)
        {
            g.setColour (juce::Colour (0xffffaa44).withAlpha (0.6f));
            g.drawVerticalLine (juce::roundToInt (openX), mb.getY(), mb.getBottom());
        }
    }
}

void GateSectionComponent::resized()
{
    auto inner = getLocalBounds().reduced (8);
    inner.removeFromTop (18); // skip section label row
    inner.removeFromTop (6);  // gap (tightened to make room for meter)
    const int knobW = inner.getWidth() / 3;
    auto knobRow = inner.removeFromTop (75);
    gateThresholdSlider .setBounds (knobRow.removeFromLeft (knobW));
    gateHysteresisSlider.setBounds (knobRow.removeFromLeft (knobW));
    glideSlider         .setBounds (knobRow.removeFromLeft (knobW));
    auto lblRow = inner.removeFromTop (18);
    gateThresholdLabel .setBounds (lblRow.removeFromLeft (knobW));
    gateHysteresisLabel.setBounds (lblRow.removeFromLeft (knobW));
    glideLabel         .setBounds (lblRow.removeFromLeft (knobW));

    inner.removeFromTop (6);  // gap before meter
    meterBounds = inner.removeFromTop (kMeterH);
}

void GateSectionComponent::setMeterValues (float envelope, bool gateOpen)
{
    meterEnvelope = envelope;
    meterGateOpen = gateOpen;
    repaint (meterBounds);
}
