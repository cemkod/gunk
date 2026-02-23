#pragma once
#include <JuceHeader.h>
#include "LookAndFeel.h"

// Base component that paints the shared rounded-rect background, border, and
// bold section label.  Subclasses override paint() and call this first if they
// need to draw additional content on top (e.g. the gate VU meter).
class LabelledSectionComponent : public juce::Component
{
public:
    explicit LabelledSectionComponent (juce::String title)
        : sectionTitle (std::move (title)) {}

    void paint (juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds();
        g.setColour (BassLookAndFeel::surface);
        g.fillRoundedRectangle (bounds.toFloat(), 6.0f);
        g.setColour (BassLookAndFeel::border);
        g.drawRoundedRectangle (bounds.toFloat(), 6.0f, 1.0f);
        g.setColour (BassLookAndFeel::text);
        g.setFont (juce::Font (11.0f, juce::Font::bold));
        g.drawText (sectionTitle, bounds.reduced (6, 4).removeFromTop (14),
                    juce::Justification::topLeft);
    }

private:
    juce::String sectionTitle;
};
