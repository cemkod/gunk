#include "FilterDisplayComponent.h"
#include "LookAndFeel.h"
#include "ParameterIDs.h"

static constexpr float kDisplayFreqMin    = 20.0f;
static constexpr float kDisplayFreqMax    = 4000.0f;
static constexpr float kDisplayDbTop      = 24.0f;
static constexpr float kDisplayDbBottom   = -48.0f;
static constexpr float kCurveStrokeWidth  = 1.5f;

FilterDisplayComponent::FilterDisplayComponent (JQGunkAudioProcessor& proc,
                                                juce::AudioProcessorValueTreeState& avts)
    : processor (proc), apvts (avts)
{
}

void FilterDisplayComponent::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds();
    const float w = (float) bounds.getWidth();
    const float h = (float) bounds.getHeight();

    // Background + border
    g.setColour (BassLookAndFeel::surfaceDark);
    g.fillRoundedRectangle (bounds.toFloat(), 4.0f);
    g.setColour (BassLookAndFeel::border);
    g.drawRoundedRectangle (bounds.toFloat(), 4.0f, 1.0f);

    // Frequency / dB display constants
    constexpr float fMin    = kDisplayFreqMin;
    constexpr float fMax    = kDisplayFreqMax;
    constexpr float dBTop   = kDisplayDbTop;
    constexpr float dBBot   = kDisplayDbBottom;
    constexpr float dBRange = dBTop - dBBot; // 72

    const float cutoffHz  = processor.getCurrentCutoffHz();
    const float Q         = juce::jlimit (0.1f, 100.0f, apvts.getRawParameterValue (ParamIDs::envResonance)->load());
    const int   filterType = (int) apvts.getRawParameterValue (ParamIDs::filterType)->load();

    const float logRatio = std::log (fMax / fMin);
    const float yZero    = h * dBTop / dBRange;
    const float xc       = w * std::log (juce::jlimit (fMin, fMax, cutoffHz) / fMin) / logRatio;

    // Grid decorations --------------------------------------------------------

    // Horizontal 0 dB line
    g.setColour (BassLookAndFeel::border.withAlpha (0.4f));
    g.drawHorizontalLine (juce::roundToInt (yZero), 0.0f, w);

    // Vertical frequency markers: 100, 500, 1000, 2000 Hz
    g.setColour (BassLookAndFeel::border.withAlpha (0.2f));
    for (float freq : { 100.0f, 500.0f, 1000.0f, 2000.0f })
    {
        const float xm = w * std::log (freq / fMin) / logRatio;
        g.drawVerticalLine (juce::roundToInt (xm), 0.0f, h);
    }

    // Cutoff position marker
    g.setColour (BassLookAndFeel::accent.withAlpha (0.25f));
    g.drawVerticalLine (juce::roundToInt (xc), 0.0f, h);

    // Response curve — evaluate actual 2-pole SVF lowpass magnitude per pixel
    // |H(f)|^2 = 1 / ((1 - (f/fc)^2)^2 + (f/(Q*fc))^2)
    const float fc       = juce::jlimit (fMin, fMax, cutoffHz);
    const int   numSteps = juce::jmax (1, (int) w);
    juce::Path curve;

    for (int i = 0; i <= numSteps; ++i)
    {
        const float x      = w * (float) i / (float) numSteps;
        const float freq   = fMin * std::exp ((float) i / (float) numSteps * logRatio);
        const float ratio  = freq / fc;
        const float ratio2 = ratio * ratio;
        const float denom  = (1.0f - ratio2) * (1.0f - ratio2) + ratio2 / (Q * Q);
        float mag2;
        if      (filterType == 1) mag2 = (ratio2 * ratio2) / denom;          // HP
        else if (filterType == 2) mag2 = (ratio2 / (Q * Q)) / denom;         // BP
        else                      mag2 = 1.0f / denom;                        // LP
        const float dB     = 10.0f * std::log10 (juce::jlimit (1e-10f, 1e10f, mag2));
        const float y      = juce::jlimit (0.0f, h, yZero - h * dB / dBRange);

        if (i == 0)
            curve.startNewSubPath (x, y);
        else
            curve.lineTo (x, y);
    }

    // Filled area under curve
    juce::Path filled = curve;
    filled.lineTo (w, h);
    filled.lineTo (0.0f, h);
    filled.closeSubPath();

    g.setColour (BassLookAndFeel::accent.withAlpha (UIConst::displayFillAlpha));
    g.fillPath (filled);

    g.setColour (BassLookAndFeel::accent.withAlpha (0.80f));
    g.strokePath (curve, juce::PathStrokeType (kCurveStrokeWidth));
}
