#pragma once

#include <JuceHeader.h>
#include "LabelledSectionComponent.h"
#include "LookAndFeel.h"
#include "UIConstants.h"
#include "PowerToggleButton.h"
#include "ParameterIDs.h"

class TransientSectionComponent : public LabelledSectionComponent
{
public:
    // Wired by the editor after construction
    std::function<bool(const juce::File&)> loadSampleFromFile;
    std::function<juce::String()>          getLoadedPath;

    TransientSectionComponent (juce::AudioProcessorValueTreeState& avts,
                               const juce::File& libraryDir)
        : LabelledSectionComponent ("TRANS"),
          pitchAttach  (avts, "transientPitch",  pitchSlider),
          levelAttach  (avts, "transientLevel",  levelSlider),
          dryAttach    (avts, "transientDry",    drySlider),
          attackAttach (avts, "transientAttack", attackSlider),
          holdAttach   (avts, "transientHold",   holdSlider),
          decayAttach  (avts, "transientDecay",  decaySlider),
          sampleLibraryDir (libraryDir),
          pwrAttach    (avts, ParamIDs::transientEnabled, pwrBtn)
    {
        pwrBtn.setClickingTogglesState (true);
        addAndMakeVisible (pwrBtn);
        BassLookAndFeel::setupRotarySlider (pitchSlider,  pitchLabel,  "PITCH",  *this);
        BassLookAndFeel::setupRotarySlider (levelSlider,  levelLabel,  "LEVEL",  *this);
        BassLookAndFeel::setupRotarySlider (drySlider,    dryLabel,    "DRY",    *this);
        BassLookAndFeel::setupRotarySlider (attackSlider, attackLabel, "ATTACK", *this);
        BassLookAndFeel::setupRotarySlider (holdSlider,   holdLabel,   "HOLD",   *this);
        BassLookAndFeel::setupRotarySlider (decaySlider,  decayLabel,  "DECAY",  *this);

        BassLookAndFeel::setButtonStyle (loadBtn, BassLookAndFeel::ButtonStyle::outline);
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
        // Power button in header bar (top-right)
        {
            const int btnH = UIConst::powerToggleH;
            const int btnW = UIConst::powerToggleW;
            auto header = getLocalBounds().removeFromTop (UIConst::sectionHeaderH);
            pwrBtn.setBounds (header.removeFromRight (btnW + UIConst::sectionInnerPad)
                                    .withSizeKeepingCentre (btnW, btnH));
        }

        auto inner = getLocalBounds().reduced (UIConst::sectionInnerPad);
        inner.removeFromTop (UIConst::sectionHeaderH); // section label row

        // Row 1: Pitch + Level + Dry
        BassLookAndFeel::placeKnobRow (inner.removeFromTop (UIConst::knobCellH),
                                       { { &pitchSlider, &pitchLabel },
                                         { &levelSlider, &levelLabel },
                                         { &drySlider,   &dryLabel } });

        inner.removeFromTop (UIConst::knobGap);

        // Row 2: Attack + Hold + Decay
        BassLookAndFeel::placeKnobRow (inner.removeFromTop (UIConst::knobCellH),
                                       { { &attackSlider, &attackLabel },
                                         { &holdSlider,   &holdLabel },
                                         { &decaySlider,  &decayLabel } });

        inner.removeFromTop (UIConst::knobGap);

        // Load button full width
        loadBtn.setBounds (inner.removeFromTop (UIConst::buttonH));
    }

private:
    void openFileChooser()
    {
        fileChooser = std::make_unique<juce::FileChooser> (
            "Load Transient Sample",
            sampleLibraryDir.isDirectory() ? sampleLibraryDir
                                           : juce::File::getSpecialLocation (juce::File::userHomeDirectory),
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
    juce::File sampleLibraryDir;  // files inside it are saved preset-relative

    PowerToggleButton pwrBtn;
    juce::AudioProcessorValueTreeState::ButtonAttachment pwrAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TransientSectionComponent)
};
