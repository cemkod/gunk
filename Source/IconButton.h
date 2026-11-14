#pragma once
#include <JuceHeader.h>
#include "LookAndFeel.h"
#include "UIConstants.h"

// A button that draws a small stroked icon built from a Path (no icon font).
// `framed` draws a rounded outline around it, as the save/delete buttons have.
class IconButton : public juce::Button
{
public:
    enum class Icon { prev, next, save, trash, close, plus };

    IconButton (Icon i, bool isFramed = false)
        : juce::Button ({}), icon (i), framed (isFramed) {}

    void setIcon (Icon i) { icon = i; repaint(); }

    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        auto b = getLocalBounds().toFloat().reduced (0.5f);
        const float r = UIConst::buttonCornerRadius;

        if (framed)
        {
            g.setColour (BassLookAndFeel::surfaceDark.brighter (down ? 0.15f : (over && isEnabled() ? 0.08f : 0.0f)));
            g.fillRoundedRectangle (b, r);
            g.setColour (BassLookAndFeel::border);
            g.drawRoundedRectangle (b, r, 1.0f);
        }
        else if (over && isEnabled())
        {
            g.setColour (BassLookAndFeel::buttonFill.withAlpha (down ? 0.9f : 0.6f));
            g.fillRoundedRectangle (b, r);
        }

        const auto col = ! isEnabled() ? BassLookAndFeel::iconDim
                                       : (over ? BassLookAndFeel::accent : BassLookAndFeel::text);
        const float s = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;   // icon box size
        const auto ib = b.withSizeKeepingCentre (s, s);
        g.setColour (col);
        g.strokePath (makePath (ib), juce::PathStrokeType (juce::jmax (1.6f, s * 0.11f),
                                                            juce::PathStrokeType::curved,
                                                            juce::PathStrokeType::rounded));
    }

private:
    juce::Path makePath (juce::Rectangle<float> b) const
    {
        juce::Path p;
        const float x = b.getX(), y = b.getY(), w = b.getWidth(), h = b.getHeight();
        switch (icon)
        {
            case Icon::prev:
                p.startNewSubPath (x + w * 0.65f, y);
                p.lineTo (x + w * 0.25f, y + h * 0.5f);
                p.lineTo (x + w * 0.65f, y + h);
                break;
            case Icon::next:
                p.startNewSubPath (x + w * 0.35f, y);
                p.lineTo (x + w * 0.75f, y + h * 0.5f);
                p.lineTo (x + w * 0.35f, y + h);
                break;
            case Icon::save:
                // Floppy: body with a clipped corner, a label slot and a shutter
                p.startNewSubPath (x, y);
                p.lineTo (x + w * 0.78f, y);
                p.lineTo (x + w, y + h * 0.22f);
                p.lineTo (x + w, y + h);
                p.lineTo (x, y + h);
                p.closeSubPath();
                p.addRectangle (x + w * 0.25f, y, w * 0.45f, h * 0.3f);
                p.addRectangle (x + w * 0.2f, y + h * 0.58f, w * 0.6f, h * 0.42f);
                break;
            case Icon::trash:
                p.startNewSubPath (x, y + h * 0.15f);
                p.lineTo (x + w, y + h * 0.15f);
                p.startNewSubPath (x + w * 0.35f, y + h * 0.15f);
                p.lineTo (x + w * 0.38f, y);
                p.lineTo (x + w * 0.62f, y);
                p.lineTo (x + w * 0.65f, y + h * 0.15f);
                p.startNewSubPath (x + w * 0.12f, y + h * 0.15f);
                p.lineTo (x + w * 0.2f, y + h);
                p.lineTo (x + w * 0.8f, y + h);
                p.lineTo (x + w * 0.88f, y + h * 0.15f);
                p.startNewSubPath (x + w * 0.4f, y + h * 0.35f);
                p.lineTo (x + w * 0.4f, y + h * 0.8f);
                p.startNewSubPath (x + w * 0.6f, y + h * 0.35f);
                p.lineTo (x + w * 0.6f, y + h * 0.8f);
                break;
            case Icon::close:
                p.startNewSubPath (x + w * 0.15f, y + h * 0.15f);
                p.lineTo (x + w * 0.85f, y + h * 0.85f);
                p.startNewSubPath (x + w * 0.85f, y + h * 0.15f);
                p.lineTo (x + w * 0.15f, y + h * 0.85f);
                break;
            case Icon::plus:
                p.startNewSubPath (x + w * 0.5f, y + h * 0.1f);
                p.lineTo (x + w * 0.5f, y + h * 0.9f);
                p.startNewSubPath (x + w * 0.1f, y + h * 0.5f);
                p.lineTo (x + w * 0.9f, y + h * 0.5f);
                break;
        }
        return p;
    }

    Icon icon;
    bool framed;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IconButton)
};
