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
          attackAttach (avts, "transientAttack", attackSlider),
          decayAttach  (avts, "transientDecay",  decaySlider)
    {
        BassLookAndFeel::setupRotarySlider (pitchSlider,  pitchLabel,  "PITCH",  *this);
        BassLookAndFeel::setupRotarySlider (levelSlider,  levelLabel,  "LEVEL",  *this);
        BassLookAndFeel::setupRotarySlider (attackSlider, attackLabel, "ATTACK", *this);
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
        auto inner = getLocalBounds().reduced (8);
        inner.removeFromTop (18); // section label row

        const int halfW = inner.getWidth() / 2;

        // Row 1: Pitch + Level
        auto knobRow1 = inner.removeFromTop (75);
        pitchSlider.setBounds (knobRow1.removeFromLeft (halfW));
        levelSlider.setBounds (knobRow1);

        auto labelRow1 = inner.removeFromTop (18);
        pitchLabel.setBounds (labelRow1.removeFromLeft (halfW));
        levelLabel.setBounds (labelRow1);

        inner.removeFromTop (4);

        // Row 2: Attack + Decay (centred as 2-of-3)
        auto knobRow2 = inner.removeFromTop (75);
        attackSlider.setBounds (knobRow2.removeFromLeft (halfW));
        decaySlider .setBounds (knobRow2);

        auto labelRow2 = inner.removeFromTop (18);
        attackLabel.setBounds (labelRow2.removeFromLeft (halfW));
        decayLabel .setBounds (labelRow2);

        inner.removeFromTop (6);

        // Load button full width
        loadBtn.setBounds (inner.removeFromTop (28));
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

    juce::Slider pitchSlider, levelSlider, attackSlider, decaySlider;
    juce::Label  pitchLabel,  levelLabel,  attackLabel,  decayLabel;

    juce::AudioProcessorValueTreeState::SliderAttachment pitchAttach, levelAttach, attackAttach, decayAttach;

    juce::TextButton loadBtn { "DROP SAMPLE" };

    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TransientSectionComponent)
};
