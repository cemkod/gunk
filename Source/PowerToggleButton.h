#pragma once
#include <JuceHeader.h>
#include "LookAndFeel.h"
#include "UIConstants.h"

// Pill-shaped on/off switch. On: yellow track, dark thumb on the right.
// Off: dark track, dim thumb on the left. Size it with UIConst::powerToggleW/H.
class PowerToggleButton : public juce::Button
{
public:
    PowerToggleButton() : juce::Button ("") {}

    void paintButton (juce::Graphics& g, bool mouseOver, bool /*buttonDown*/) override
    {
        const bool on = getToggleState();

        // Fit a pill (2:1 or wider) into the bounds, centred
        auto b = getLocalBounds().toFloat().reduced (0.5f);
        const float h = juce::jmin (b.getHeight(), b.getWidth() * 0.5f);
        b = b.withSizeKeepingCentre (b.getWidth(), h);
        const float r = h * 0.5f;

        if (on)
        {
            g.setColour (mouseOver ? BassLookAndFeel::accent.brighter (0.1f) : BassLookAndFeel::accent);
            g.fillRoundedRectangle (b, r);
        }
        else
        {
            g.setColour (BassLookAndFeel::surfaceDark);
            g.fillRoundedRectangle (b, r);
            g.setColour (mouseOver ? BassLookAndFeel::textDim : BassLookAndFeel::borderDim);
            g.drawRoundedRectangle (b.reduced (0.5f), r - 0.5f, 1.0f);
        }

        const float pad    = juce::jmax (1.5f, h * 0.15f);
        const float thumbD = h - pad * 2.0f;
        const float thumbX = on ? b.getRight() - pad - thumbD : b.getX() + pad;
        g.setColour (on ? BassLookAndFeel::accentText : BassLookAndFeel::iconDim);
        g.fillEllipse (thumbX, b.getY() + pad, thumbD, thumbD);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PowerToggleButton)
};
