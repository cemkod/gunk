#pragma once

#include <JuceHeader.h>
#include "SectionPanel.h"
#include "LookAndFeel.h"
#include "UIConstants.h"
#include "ParameterIDs.h"
#include "SegmentedControl.h"

// OUTPUT: master volume, the dry input level and path, and the stereo mono-below crossover.
class OutputSectionComponent : public SectionPanel
{
public:
    explicit OutputSectionComponent (juce::AudioProcessorValueTreeState& avts)
        : SectionPanel ("OUTPUT"),
          volAttach  (avts, "masterVolume", volSlider),
          dryAttach  (avts, "dryLevel", drySlider),
          monoAttach (avts, ParamIDs::stereoMonoBelow, monoSlider)
    {
        BassLookAndFeel::setupRotarySlider (volSlider,  volLabel,  "VOLUME",     *this);
        BassLookAndFeel::setupRotarySlider (drySlider,  dryLabel,  "DRY",        *this);
        BassLookAndFeel::setupRotarySlider (monoSlider, monoLabel, "MONO BELOW", *this);
        drySlider .setTooltip ("Level of the unprocessed input in the mix");
        monoSlider.setTooltip ("Unison WIDTH is kept mono below this frequency");

        BassLookAndFeel::setupCaption (dryPathLabel, "DRY PATH", *this);
        dryPathSwitch.attachToParameter (avts, ParamIDs::dryRouting);
        addAndMakeVisible (dryPathSwitch);
        dryPathLabel.setTooltip ("Through FX: the dry input goes through Drive and the Filter with the synth");
    }

    void paint (juce::Graphics& g) override
    {
        SectionPanel::paint (g);
        // Divider between the knobs and the dry path switch
        g.setColour (BassLookAndFeel::border);
        g.fillRect (dividerX, getBodyBounds().getY() + 4, 1, getBodyBounds().getHeight() - 8);
    }

private:
    void layoutBody (juce::Rectangle<int> body) override
    {
        using K = BassLookAndFeel::KnobSize;
        auto path = body.removeFromRight (150);
        dividerX = path.getX() - 8;
        body.removeFromRight (16);

        path = path.withSizeKeepingCentre (path.getWidth(), UIConst::captionH + 6 + 34);
        dryPathLabel.setBounds (path.removeFromTop (UIConst::captionH));
        path.removeFromTop (6);
        dryPathSwitch.setBounds (path);

        const int colW = body.getWidth() / 3;
        BassLookAndFeel::placeKnob (volSlider,  volLabel,  body.removeFromLeft (colW), K::hero);
        BassLookAndFeel::placeKnob (drySlider,  dryLabel,  body.removeFromLeft (colW));
        BassLookAndFeel::placeKnob (monoSlider, monoLabel, body);
    }

    juce::Slider volSlider, drySlider, monoSlider;
    juce::Label  volLabel,  dryLabel,  monoLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment volAttach, dryAttach, monoAttach;

    juce::Label      dryPathLabel;
    SegmentedControl dryPathSwitch { { "Clean", "Through FX" } };

    int dividerX = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OutputSectionComponent)
};
