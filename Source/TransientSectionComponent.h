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
          levelAttach  (avts, "transientLevel",  levelSlider),
          attackAttach (avts, "transientAttack", attackSlider),
          decayAttach  (avts, "transientDecay",  decaySlider)
    {
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

        const int knobW = inner.getWidth() / 3;

        // Knob row
        auto knobRow = inner.removeFromTop (75);
        levelSlider .setBounds (knobRow.removeFromLeft (knobW));
        attackSlider.setBounds (knobRow.removeFromLeft (knobW));
        decaySlider .setBounds (knobRow);

        // Label row
        auto labelRow = inner.removeFromTop (18);
        levelLabel .setBounds (labelRow.removeFromLeft (knobW));
        attackLabel.setBounds (labelRow.removeFromLeft (knobW));
        decayLabel .setBounds (labelRow);

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

    juce::Slider levelSlider, attackSlider, decaySlider;
    juce::Label  levelLabel,  attackLabel,  decayLabel;

    juce::AudioProcessorValueTreeState::SliderAttachment levelAttach, attackAttach, decayAttach;

    juce::TextButton loadBtn { "DROP SAMPLE" };

    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TransientSectionComponent)
};
