#pragma once
#include <JuceHeader.h>
#include "LookAndFeel.h"
#include "UIConstants.h"

class PowerToggleButton : public juce::Button
{
public:
    PowerToggleButton() : juce::Button ("") {}

    void paintButton (juce::Graphics& g, bool /*mouseOver*/, bool /*buttonDown*/) override
    {
        const bool on = getToggleState();
        const auto b  = getLocalBounds().toFloat().reduced (0.5f);

        g.setColour (on ? BassLookAndFeel::surface : BassLookAndFeel::surfaceDark);
        g.fillRoundedRectangle (b, UIConst::buttonCornerRadius);
        g.setColour (on ? BassLookAndFeel::accent : BassLookAndFeel::borderDim);
        g.drawRoundedRectangle (b, UIConst::buttonCornerRadius, 1.0f);

        const auto ic = b.reduced (1.5f);
        const float cx = ic.getCentreX();
        const float cy = ic.getCentreY();
        const float r  = juce::jmin (ic.getWidth(), ic.getHeight()) * 0.38f;

        g.setColour (on ? BassLookAndFeel::accent : BassLookAndFeel::iconDim);

        // Arc: nearly full circle with gap at top (12 o'clock).
        // JUCE addCentredArc: 0 = 12 o'clock, positive = clockwise.
        juce::Path arc;
        const float top      = 0.0f;
        const float gapHalf  = juce::degreesToRadians (25.0f);
        arc.addCentredArc (cx, cy, r, r, 0.0f,
                           top + gapHalf,
                           top - gapHalf + juce::MathConstants<float>::twoPi,
                           true);
        g.strokePath (arc, juce::PathStrokeType (1.3f));

        // Vertical line: from just inside the circle top, up through the gap.
        g.drawLine (cx, cy - r * 0.3f, cx, cy - r * 1.15f, 1.3f);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PowerToggleButton)
};
