#include "GateSectionComponent.h"
#include "LookAndFeel.h"

static constexpr float        kMeterDbMin              = -60.0f;
static const     juce::Colour kGateLedOnColor          { 0xff44ff88 };
static const     juce::Colour kGateThresholdMarkerColor { 0xffff6633 };
static const     juce::Colour kGateOpenThresholdColor   { 0xffffaa44 };

GateSectionComponent::GateSectionComponent (juce::AudioProcessorValueTreeState& avts)
    : LabelledSectionComponent ("GATE/TRACKING"),
      apvts (avts),
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
}

GateSectionComponent::~GateSectionComponent()
{
    setLookAndFeel (nullptr);
}

void GateSectionComponent::paint (juce::Graphics& g)
{
    LabelledSectionComponent::paint (g);

    // --- VU meter strip ---
    {
        auto mb = meterBounds.toFloat();

        // Background
        g.setColour (BassLookAndFeel::surfaceDark);
        g.fillRoundedRectangle (mb, 3.0f);

        // dB-scale helper: maps linear amplitude to 0..1 fill fraction (-60..0 dBFS)
        auto toFill = [] (float linear) -> float {
            const float dB = 20.0f * std::log10 (juce::jmax (linear, 1e-10f));
            return juce::jlimit (0.0f, 1.0f, (dB - kMeterDbMin) / -kMeterDbMin);
        };

        // Level bar
        const float fillFrac = toFill (meterEnvelope);
        if (fillFrac > 0.0f)
        {
            const juce::Colour barColour = meterGateOpen
                ? kGateLedOnColor
                : kGateLedOnColor.withAlpha (0.4f);
            g.setColour (barColour);
            g.fillRoundedRectangle (mb.withWidth (mb.getWidth() * fillFrac), 3.0f);
        }

        // Threshold marker lines (read live from APVTS)
        const float thresh = *apvts.getRawParameterValue ("gateThreshold");
        const float hyst   = *apvts.getRawParameterValue ("gateHysteresis");

        // Close threshold (orange-red)
        const float threshX = mb.getX() + toFill (thresh) * mb.getWidth();
        g.setColour (kGateThresholdMarkerColor.withAlpha (0.9f));
        g.drawVerticalLine (juce::roundToInt (threshX), mb.getY(), mb.getBottom());

        // Open threshold (amber, dimmer) — hyst is in dB
        const float openX = mb.getX() + toFill (thresh * std::pow (10.0f, hyst / 20.0f)) * mb.getWidth();
        if (openX > threshX + 1.0f)
        {
            g.setColour (kGateOpenThresholdColor.withAlpha (0.6f));
            g.drawVerticalLine (juce::roundToInt (openX), mb.getY(), mb.getBottom());
        }
    }
}

void GateSectionComponent::resized()
{
    auto inner = getLocalBounds().reduced (8);
    inner.removeFromTop (18); // skip section label row
    inner.removeFromTop (6);  // gap (tightened to make room for meter)
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

    inner.removeFromTop (6);  // gap before meter
    meterBounds = inner.removeFromTop (kMeterH);
}

void GateSectionComponent::setMeterValues (float envelope, bool gateOpen)
{
    meterEnvelope = envelope;
    meterGateOpen = gateOpen;
    repaint (meterBounds);
}
