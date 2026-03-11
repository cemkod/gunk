#pragma once

#include <JuceHeader.h>
#include "LabelledSectionComponent.h"
#include "LookAndFeel.h"
#include "UIConstants.h"

class TransientSectionComponent : public LabelledSectionComponent
{
public:
    // Wired by the editor after construction
    std::function<bool(const juce::File&)> loadSampleFromFile;
    std::function<juce::String()>          getLoadedPath;

    TransientSectionComponent (juce::AudioProcessorValueTreeState& avts)
        : LabelledSectionComponent ("TRANS"),
          pitchAttach  (avts, "transientPitch",  pitchSlider),
          levelAttach  (avts, "transientLevel",  levelSlider),
          dryAttach    (avts, "transientDry",    drySlider),
          attackAttach (avts, "transientAttack", attackSlider),
          holdAttach   (avts, "transientHold",   holdSlider),
          decayAttach  (avts, "transientDecay",  decaySlider)
    {
        BassLookAndFeel::setupRotarySlider (pitchSlider,  pitchLabel,  "PITCH",  *this);
        BassLookAndFeel::setupRotarySlider (levelSlider,  levelLabel,  "LEVEL",  *this);
        BassLookAndFeel::setupRotarySlider (drySlider,    dryLabel,    "DRY",    *this);
        BassLookAndFeel::setupRotarySlider (attackSlider, attackLabel, "ATTACK", *this);
        BassLookAndFeel::setupRotarySlider (holdSlider,   holdLabel,   "HOLD",   *this);
        BassLookAndFeel::setupRotarySlider (decaySlider,  decayLabel,  "DECAY",  *this);

        loadBtn.setColour (juce::TextButton::buttonColourId,  BassLookAndFeel::surface);
        loadBtn.setColour (juce::TextButton::textColourOffId, BassLookAndFeel::iconDim);
        loadBtn.onClick = [this] { openFileChooser(); };
        addAndMakeVisible (loadBtn);

        updateButtonStates();
    }

    ~TransientSectionComponent() override
    {
        setLookAndFeel (nullptr);
    }

    void updateButtonStates()
    {
        if (getLoadedPath)
        {
            const juce::String path = getLoadedPath();
            if (path.isNotEmpty())
                loadBtn.setButtonText (juce::File (path).getFileName());
            else
                loadBtn.setButtonText ("DROP SAMPLE");
        }
        else
        {
            loadBtn.setButtonText ("DROP SAMPLE");
        }
    }

    void resized() override
    {
        auto inner = getLocalBounds().reduced (UIConst::sectionInnerPad);
        inner.removeFromTop (UIConst::sectionHeaderH); // section label row

        const int thirdW = inner.getWidth() / 3;

        // Row 1: Pitch + Level + Dry
        auto knobRow1 = inner.removeFromTop (UIConst::knobRowH);
        pitchSlider.setBounds (knobRow1.removeFromLeft (thirdW));
        levelSlider.setBounds (knobRow1.removeFromLeft (thirdW));
        drySlider  .setBounds (knobRow1);

        auto labelRow1 = inner.removeFromTop (UIConst::knobLabelH);
        pitchLabel.setBounds (labelRow1.removeFromLeft (thirdW));
        levelLabel.setBounds (labelRow1.removeFromLeft (thirdW));
        dryLabel  .setBounds (labelRow1);

        inner.removeFromTop (UIConst::knobGap);

        // Row 2: Attack + Hold + Decay
        auto knobRow2 = inner.removeFromTop (UIConst::knobRowH);
        attackSlider.setBounds (knobRow2.removeFromLeft (thirdW));
        holdSlider  .setBounds (knobRow2.removeFromLeft (thirdW));
        decaySlider .setBounds (knobRow2);

        auto labelRow2 = inner.removeFromTop (UIConst::knobLabelH);
        attackLabel.setBounds (labelRow2.removeFromLeft (thirdW));
        holdLabel  .setBounds (labelRow2.removeFromLeft (thirdW));
        decayLabel .setBounds (labelRow2);

        inner.removeFromTop (UIConst::knobGap);

        // Load button full width
        loadBtn.setBounds (inner.removeFromTop (UIConst::buttonH));
    }

private:
    void openFileChooser()
    {
        fileChooser = std::make_unique<juce::FileChooser> (
            "Load Transient Sample",
            juce::File::getSpecialLocation (juce::File::userHomeDirectory),
            "*.wav;*.aif;*.aiff");

        fileChooser->launchAsync (
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this] (const juce::FileChooser& fc)
            {
                const auto results = fc.getResults();
                if (results.isEmpty()) return;
                const juce::File chosen = results[0];
                if (loadSampleFromFile && loadSampleFromFile (chosen))
                    updateButtonStates();
            });
    }

    juce::Slider pitchSlider, levelSlider, drySlider, attackSlider, holdSlider, decaySlider;
    juce::Label  pitchLabel,  levelLabel,  dryLabel,  attackLabel,  holdLabel,  decayLabel;

    juce::AudioProcessorValueTreeState::SliderAttachment pitchAttach, levelAttach, dryAttach, attackAttach, holdAttach, decayAttach;

    juce::TextButton loadBtn { "DROP SAMPLE" };

    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TransientSectionComponent)
};
