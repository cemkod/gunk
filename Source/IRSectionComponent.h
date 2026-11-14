#pragma once

#include <JuceHeader.h>
#include "LabelledSectionComponent.h"
#include "LookAndFeel.h"
#include "UIConstants.h"
#include "PowerToggleButton.h"

class IRSectionComponent : public LabelledSectionComponent
{
public:
    // Wired by the editor after construction
    std::function<bool(const juce::File&)> loadIRFromFile;
    std::function<juce::String()>          getLoadedPath;

    IRSectionComponent (juce::AudioProcessorValueTreeState& avts,
                        const juce::File& factoryIRDir)
        : LabelledSectionComponent ("IR"),
          mixAttach    (avts, "irMix",    mixSlider),
          makeupAttach (avts, "irMakeup", makeupSlider),
          irLibraryDir (factoryIRDir)
    {
        BassLookAndFeel::setupRotarySlider (mixSlider,    mixLabel,    "MIX",    *this);
        BassLookAndFeel::setupRotarySlider (makeupSlider, makeupLabel, "MAKEUP", *this);

        enableBtn.setClickingTogglesState (true);
        addAndMakeVisible (enableBtn);
        enableAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
            avts, "irEnabled", enableBtn);

        setupIRCombo (factoryIRDir);
        addAndMakeVisible (irCombo);
    }

    ~IRSectionComponent() override
    {
        setLookAndFeel (nullptr);
    }

    void updateButtonStates()
    {
        syncIRCombo();
    }

    void resized() override
    {
        {
            const int btnW = UIConst::powerToggleW, btnH = UIConst::powerToggleH;
            auto header = getLocalBounds().removeFromTop (UIConst::sectionHeaderH);
            auto btnArea = header.removeFromRight (UIConst::sectionInnerPad + btnW)
                                 .withSizeKeepingCentre (btnW, btnH);
            enableBtn.setBounds (btnArea);
        }

        auto inner = getLocalBounds().reduced (UIConst::sectionInnerPad);
        inner.removeFromTop (UIConst::sectionHeaderH);

        inner.removeFromTop (UIConst::knobGap);
        auto knobRow = inner.removeFromTop (UIConst::knobRowH);
        const int knobW = knobRow.getWidth() / 2;
        mixSlider   .setBounds (knobRow.removeFromLeft (knobW));
        makeupSlider.setBounds (knobRow);

        auto labelRow = inner.removeFromTop (UIConst::knobLabelH);
        mixLabel   .setBounds (labelRow.removeFromLeft (knobW));
        makeupLabel.setBounds (labelRow);

        inner.removeFromTop (UIConst::sectionInnerPad);
        irCombo.setBounds (inner.removeFromTop (UIConst::buttonH));
    }

private:
    void setupIRCombo (const juce::File& factoryIRDir)
    {
        irCombo.addItem ("Load IR...", 99);

        if (factoryIRDir.isDirectory())
        {
            auto files = factoryIRDir.findChildFiles (juce::File::findFiles, false, "*.wav;*.aif;*.aiff");
            files.sort();

            if (! files.isEmpty())
            {
                juce::PopupMenu factoryMenu;
                for (auto& f : files)
                {
                    factoryIRFiles.add (f);
                    factoryMenu.addItem (100 + factoryIRFiles.size() - 1,
                                        f.getFileNameWithoutExtension());
                }
                irCombo.addSeparator();
                irCombo.getRootMenu()->addSubMenu ("Factory IRs", factoryMenu);
            }
        }

        irCombo.onChange = [this] { onComboChanged(); };
        irCombo.setSelectedId (0, juce::dontSendNotification);
    }

    void syncIRCombo()
    {
        const juce::String path = getLoadedPath ? getLoadedPath() : juce::String{};

        if (path.isNotEmpty())
        {
            juce::String name = juce::File (path).getFileNameWithoutExtension();
            // Rebuild if custom item doesn't exist or name changed
            if (irCombo.getItemText (irCombo.getNumItems() - 1) != name
                || irCombo.getItemId  (irCombo.getNumItems() - 1) != 50)
            {
                irCombo.clear (juce::dontSendNotification);
                irCombo.addItem ("Load IR...", 99);
                addFactorySubmenu();
                irCombo.addSeparator();
                irCombo.addItem (name, 50);
            }
            irCombo.setSelectedId (50, juce::dontSendNotification);
        }
        else
        {
            irCombo.setSelectedId (0, juce::dontSendNotification);
        }
    }

    void addFactorySubmenu()
    {
        if (factoryIRFiles.isEmpty()) return;

        juce::PopupMenu factoryMenu;
        for (int i = 0; i < factoryIRFiles.size(); ++i)
            factoryMenu.addItem (100 + i, factoryIRFiles[i].getFileNameWithoutExtension());
        irCombo.getRootMenu()->addSubMenu ("Factory IRs", factoryMenu);
    }

    void onComboChanged()
    {
        const int id = irCombo.getSelectedId();
        if (id == 99)
        {
            irCombo.setSelectedId (0, juce::dontSendNotification);
            openFileChooser();
        }
        else if (id >= 100 && id < 100 + factoryIRFiles.size())
        {
            irCombo.setSelectedId (0, juce::dontSendNotification);
            if (loadIRFromFile)
                loadIRFromFile (factoryIRFiles[id - 100]);
            syncIRCombo();
        }
        // id 50 = already-loaded IR — no action
    }

    void openFileChooser()
    {
        fileChooser = std::make_unique<juce::FileChooser> (
            "Load Impulse Response",
            irLibraryDir.isDirectory() ? irLibraryDir
                                       : juce::File::getSpecialLocation (juce::File::userHomeDirectory),
            "*.wav;*.aif;*.aiff");

        fileChooser->launchAsync (
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this] (const juce::FileChooser& fc)
            {
                const auto results = fc.getResults();
                if (results.isEmpty()) return;
                if (loadIRFromFile && loadIRFromFile (results[0]))
                    syncIRCombo();
            });
    }

    juce::Slider mixSlider;
    juce::Label  mixLabel    { "", "MIX" };
    juce::Slider makeupSlider;
    juce::Label  makeupLabel { "", "MAKEUP" };
    juce::AudioProcessorValueTreeState::SliderAttachment mixAttach;
    juce::AudioProcessorValueTreeState::SliderAttachment makeupAttach;

    PowerToggleButton enableBtn;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableAttach;

    juce::ComboBox           irCombo;
    juce::Array<juce::File>  factoryIRFiles;

    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::File irLibraryDir;  // files inside it are saved preset-relative

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IRSectionComponent)
};
