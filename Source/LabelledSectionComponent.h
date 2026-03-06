#pragma once
#include <JuceHeader.h>
#include "LookAndFeel.h"
#include "UIConstants.h"

// Base component that paints the shared rounded-rect background, border, and
// a Surge XT-style dark header bar with section title.
class LabelledSectionComponent : public juce::Component
{
public:
    explicit LabelledSectionComponent (juce::String title)
        : sectionTitle (std::move (title)) {}

    void setSuppressBorder (bool b) { suppressBorder_ = b; }

    void paint (juce::Graphics& g) override
    {
        if (suppressBorder_) return;
        const auto bounds = getLocalBounds();
        const float r = UIConst::sectionCornerRadius;

        // Body fill + border
        g.setColour (BassLookAndFeel::surface);
        g.fillRoundedRectangle (bounds.toFloat(), r);
        g.setColour (BassLookAndFeel::border);
        g.drawRoundedRectangle (bounds.toFloat(), r, 1.0f);

        // Dark header bar clipped to rounded top
        {
            juce::Graphics::ScopedSaveState ss (g);
            juce::Path clip;
            clip.addRoundedRectangle (bounds.toFloat(), r);
            g.reduceClipRegion (clip);

            g.setColour (BassLookAndFeel::headerBar);
            g.fillRect (bounds.withHeight (UIConst::sectionHeaderH));
        }

        // Section title in header bar
        g.setColour (BassLookAndFeel::textDim);
        g.setFont (juce::Font (UIConst::fontSectionTitle, juce::Font::bold));
        g.drawText (sectionTitle,
                    getLocalBounds().removeFromTop (UIConst::sectionHeaderH)
                                    .reduced (UIConst::sectionInnerPad, 0),
                    juce::Justification::centredLeft);
    }

private:
    juce::String sectionTitle;
    bool suppressBorder_ = false;
};
