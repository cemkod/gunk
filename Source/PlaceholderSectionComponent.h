#pragma once

#include <JuceHeader.h>
#include "LabelledSectionComponent.h"
#include "LookAndFeel.h"

class PlaceholderSectionComponent : public LabelledSectionComponent
{
public:
    explicit PlaceholderSectionComponent (const juce::String& title)
        : LabelledSectionComponent (title) {}

    void paint (juce::Graphics& g) override
    {
        LabelledSectionComponent::paint (g);  // draw frame + title

        g.setColour (BassLookAndFeel::textDim.withAlpha (0.4f));
        g.setFont (juce::Font (UIConst::uiFontSize));
        g.drawText ("coming soon", getLocalBounds(), juce::Justification::centred);
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlaceholderSectionComponent)
};
