#include "CombinedOscSectionComponent.h"
#include "LookAndFeel.h"
#include "ParameterIDs.h"

static juce::File getWavetablesDir()
{
    auto d = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                 .getChildFile ("JQGunk/Wavetables");
    if (d.isDirectory()) return d;
   #if ! JUCE_WINDOWS
    juce::File sys ("/usr/share/JQGunk/Wavetables");
    if (sys.isDirectory()) return sys;
   #endif
    return d;
}

CombinedOscSectionComponent::CombinedOscSectionComponent (
    juce::AudioProcessorValueTreeState& avts,
    const OscParamIds& ids1,
    const OscParamIds& ids2)
    : SectionPanel ("OSCILLATORS"),
      apvts (avts),
      osc1     (ids1, avts),
      osc2Comp (ids2, avts)
{
    paramIds[0] = ids1;
    paramIds[1] = ids2;

    // Populate factory wavetable list (scan Wavetables/Surge or Wavetables root)
    {
        auto wavetablesRoot = getWavetablesDir();
        auto surgeDir = wavetablesRoot.getChildFile ("Surge");
        auto scanRoot = surgeDir.isDirectory() ? surgeDir : wavetablesRoot;

        juce::Array<juce::File> categoryDirs;
        scanRoot.findChildFiles (categoryDirs, juce::File::findDirectories, false);
        categoryDirs.sort();

        for (auto& catDir : categoryDirs)
        {
            juce::Array<juce::File> files;
            catDir.findChildFiles (files, juce::File::findFiles, false, "*.wav;*.wt");
            files.sort();
            factoryWavetables.addArray (files);
        }
    }

    // Header tabs, each with its section's enable switch
    const juce::String enableIds[3] { ParamIDs::osc1Enabled, ParamIDs::osc2Enabled, ParamIDs::subEnabled };
    for (int i = 0; i < 3; ++i)
    {
        auto& t = tabs[(size_t) i];
        tabPowerAttach[(size_t) i] = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
            avts, enableIds[i], t.power);
        enabledParams[(size_t) i] = avts.getRawParameterValue (enableIds[i]);
        t.onSelect = [this, i] { selectTab (i); };
        addAndMakeVisible (t);
    }

    addAndMakeVisible (waveformDisplay);
    addAndMakeVisible (unisonDisplay);

    // Waveform combos
    setupWaveCombo (waveCombo1, 0);
    setupWaveCombo (waveCombo2, 1);

    // Octave combos
    for (auto* combo : { &octCombo1, &octCombo2 })
    {
        combo->addItemList ({ "-2", "-1", "0", "+1", "+2" }, 1);
        addAndMakeVisible (combo);
    }
    // Attach after the items exist, so the initial selection is applied.
    octAttach1 = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (avts, ids1.octaveShift, octCombo1);
    octAttach2 = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (avts, ids2.octaveShift, octCombo2);
    BassLookAndFeel::setupCaption (octLabel, "OCTAVE", *this, juce::Justification::centred);
    BassLookAndFeel::setupCaption (unisonCaption, "UNISON", *this, juce::Justification::centred);

    addAndMakeVisible (osc1);
    addAndMakeVisible (osc2Comp);

    // SUB page
    BassLookAndFeel::setupRotarySlider (subLevelSlider, subLevelLabel, "LEVEL", *this);
    subLevelAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        avts, ParamIDs::subLevel, subLevelSlider);
    subOctCombo.addItemList ({ "-2", "-1", "0", "+1" }, 1);
    subOctAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        avts, ParamIDs::subOctave, subOctCombo);
    addAndMakeVisible (subOctCombo);
    BassLookAndFeel::setupCaption (subOctLabel, "OCTAVE", *this, juce::Justification::centred);
    skipFilterBtn.setClickingTogglesState (true);
    BassLookAndFeel::setButtonStyle (skipFilterBtn, BassLookAndFeel::ButtonStyle::outline);
    skipFilterBtn.setTooltip ("The sub skips the filter and joins the mix clean");
    skipFilterAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        avts, ParamIDs::subBypassFilter, skipFilterBtn);
    addAndMakeVisible (skipFilterBtn);

    selectTab (0);
}

void CombinedOscSectionComponent::selectTab (int idx)
{
    selectedTab = idx;
    const bool isOsc = idx != kSubTab;

    for (int i = 0; i < 3; ++i)
    {
        tabs[(size_t) i].selected = (i == idx);
        tabs[(size_t) i].repaint();
    }

    osc1    .setVisible (idx == 0);
    osc2Comp.setVisible (idx == 1);
    waveCombo1.setVisible (idx == 0);
    waveCombo2.setVisible (idx == 1);
    octCombo1 .setVisible (idx == 0);
    octCombo2 .setVisible (idx == 1);
    octLabel     .setVisible (isOsc);
    unisonCaption.setVisible (isOsc);
    unisonDisplay.setVisible (isOsc);

    subLevelSlider.setVisible (! isOsc);
    subLevelLabel .setVisible (! isOsc);
    subOctCombo   .setVisible (! isOsc);
    subOctLabel   .setVisible (! isOsc);
    skipFilterBtn .setVisible (! isOsc);

    if (isOsc)
    {
        waveformDisplay.getDisplayFrame = getDisplayFrame[(size_t) idx];
        unisonDisplay.getVoices = [this, idx] {
            auto* p = apvts.getParameter (paramIds[idx].unisonVoices);
            return p ? p->getValue() * 7.0f + 1.0f : 1.0f;
        };
        unisonDisplay.getBlend = [this, idx] { return getModBlend ? getModBlend (idx) : 1.0f; };
        unisonDisplay.getWidth = [this, idx] { return getModWidth ? getModWidth (idx) : 0.0f; };
        syncWaveCombo (idx);
    }
    else
    {
        // The sub is a sine
        waveformDisplay.getDisplayFrame = [] (std::vector<float>& buf)
        {
            const auto n = buf.size();
            for (size_t i = 0; i < n; ++i)
                buf[i] = std::sin (juce::MathConstants<float>::twoPi * (float) i / (float) n);
        };
    }
    repaintDisplays();

    // Dimming follows the selected tab's enable switch
    setDimmed (false);
    refreshDimmed();
    relayout();
}

void CombinedOscSectionComponent::refreshDimmed()
{
    if (auto* p = enabledParams[(size_t) selectedTab])
        setDimmed (p->load (std::memory_order_relaxed) < 0.5f);
}

void CombinedOscSectionComponent::addFactorySubmenu (juce::ComboBox& combo)
{
    if (factoryWavetables.isEmpty())
        return;

    auto wavetablesRoot = getWavetablesDir();
    auto surgeDir = wavetablesRoot.getChildFile ("Surge");
    auto scanRoot = surgeDir.isDirectory() ? surgeDir : wavetablesRoot;

    juce::Array<juce::File> categoryDirs;
    scanRoot.findChildFiles (categoryDirs, juce::File::findDirectories, false);
    categoryDirs.sort();

    juce::PopupMenu factoryMenu;
    bool anyCategory = false;

    for (auto& catDir : categoryDirs)
    {
        juce::Array<juce::File> files;
        catDir.findChildFiles (files, juce::File::findFiles, false, "*.wav;*.wt");
        files.sort();

        if (files.isEmpty())
            continue;

        juce::PopupMenu catMenu;
        for (auto& f : files)
        {
            int id = 100 + factoryWavetables.indexOf (f);
            if (id >= 100)
                catMenu.addItem (id, f.getFileNameWithoutExtension());
        }
        factoryMenu.addSubMenu (catDir.getFileName(), catMenu);
        anyCategory = true;
    }

    if (anyCategory)
    {
        combo.addSeparator();
        combo.getRootMenu()->addSubMenu ("Factory Wavetables", factoryMenu);
    }
}

void CombinedOscSectionComponent::setupWaveCombo (juce::ComboBox& combo, int oscIdx)
{
    combo.addItem ("Triangle", 1);
    combo.addItem ("Square",   2);
    combo.addItem ("Sawtooth", 3);
    combo.addSeparator();
    combo.addItem ("Load Wavetable", 99);
    addFactorySubmenu (combo);

    combo.onChange = [this, &combo, oscIdx]
    {
        const int id = combo.getSelectedId();
        if (id >= 1 && id <= 3)
        {
            setWaveformParam (oscIdx, id - 1);
        }
        else if (id == 99)
        {
            // Reset selection back to current before opening dialog
            combo.setSelectedId (0, juce::dontSendNotification);
            openWavFileDialog (oscIdx);
        }
        else if (id >= 100 && id < 100 + factoryWavetables.size())
        {
            combo.setSelectedId (0, juce::dontSendNotification);
            if (loadWavetableFromFile[(size_t) oscIdx])
                loadWavetableFromFile[(size_t) oscIdx] (factoryWavetables[id - 100]);
        }
        // id 50 = loaded custom wavetable, already active
    };

    addAndMakeVisible (combo);
}

void CombinedOscSectionComponent::syncWaveCombo (int oscIdx)
{
    auto& combo = (oscIdx == 0) ? waveCombo1 : waveCombo2;
    const auto idx = (size_t) oscIdx;

    // If a custom wavetable is active, show it
    if (isCustomWaveformActive[idx] && isCustomWaveformActive[idx]())
    {
        juce::String name = getWavetableName[idx] ? getWavetableName[idx]() : "Custom";
        // Rebuild if the custom item doesn't exist or name changed
        if (combo.getItemText (combo.getNumItems() - 1) != name
            || combo.getItemId  (combo.getNumItems() - 1) != 50)
        {
            combo.clear (juce::dontSendNotification);
            combo.addItem ("Triangle", 1);
            combo.addItem ("Square",   2);
            combo.addItem ("Sawtooth", 3);
            combo.addSeparator();
            combo.addItem ("Load Wavetable", 99);
            addFactorySubmenu (combo);
            combo.addSeparator();
            combo.addItem (name, 50);
        }
        combo.setSelectedId (50, juce::dontSendNotification);
        return;
    }

    // Sync to APVTS waveform param (0=Triangle,1=Square,2=Sawtooth -> IDs 1,2,3)
    auto* p = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (paramIds[oscIdx].waveform));
    if (p)
        combo.setSelectedId ((int) p->getIndex() + 1, juce::dontSendNotification);
}

void CombinedOscSectionComponent::setWaveformParam (int oscIdx, int waveIdx)
{
    auto* p = dynamic_cast<juce::AudioParameterChoice*> (
        apvts.getParameter (paramIds[oscIdx].waveform));
    if (p) *p = waveIdx;
}

void CombinedOscSectionComponent::openWavFileDialog (int oscIdx)
{
    fileChooser = std::make_unique<juce::FileChooser> (
        "Select a WAV or WT file",
        juce::File::getSpecialLocation (juce::File::userHomeDirectory),
        "*.wav;*.wt");
    fileChooser->launchAsync (
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, oscIdx] (const juce::FileChooser& fc)
        {
            auto results = fc.getResults();
            if (results.isEmpty()) return;
            const bool ok = loadWavetableFromFile[(size_t) oscIdx]
                            && loadWavetableFromFile[(size_t) oscIdx] (results.getFirst());
            if (ok)
                syncWaveCombo (oscIdx);
            else
                juce::AlertWindow::showMessageBoxAsync (
                    juce::AlertWindow::WarningIcon, "Load failed",
                    "Could not read the selected WAV or WT file.", "OK");
        });
}

void CombinedOscSectionComponent::updateButtonStates()
{
    osc1.getNumFrames     = getNumFrames[0];
    osc2Comp.getNumFrames = getNumFrames[1];

    // Keep display callbacks current (also fixes initial null on first load)
    if (selectedTab != kSubTab)
        waveformDisplay.getDisplayFrame = getDisplayFrame[(size_t) selectedTab];

    syncWaveCombo (0);
    syncWaveCombo (1);

    if (selectedTab == 0)
        osc1.updateButtonStates();
    else if (selectedTab == 1)
        osc2Comp.updateButtonStates();
}

void CombinedOscSectionComponent::paint (juce::Graphics& g)
{
    SectionPanel::paint (g);

    if (separatorX >= 0 && selectedTab != kSubTab)
    {
        const auto body = getBodyBounds();
        g.setColour (BassLookAndFeel::border);
        g.fillRect (separatorX, body.getY() + 2, 1, body.getHeight() - 4);
    }
}

void CombinedOscSectionComponent::layoutBody (juce::Rectangle<int> body)
{
    // Tabs on the right of the header
    {
        auto header = getHeaderBounds().reduced (UIConst::sectionInnerPad, 4);
        constexpr int tabW = 112, gap = 6;
        for (int i = 2; i >= 0; --i)
        {
            tabs[(size_t) i].setBounds (header.removeFromRight (tabW));
            header.removeFromRight (gap);
        }
    }

    const int knobH = BassLookAndFeel::knobCellHeight (BassLookAndFeel::KnobSize::standard);
    constexpr int colGap = 24;

    // Two columns: waveform and the main knobs (60 %) | unison display and its knobs (40 %)
    auto left  = body.removeFromLeft ((body.getWidth() - colGap) * 60 / 100);
    body.removeFromLeft (colGap);
    auto right = body;
    separatorX = left.getRight() + colGap / 2;

    auto leftKnobs  = left .removeFromBottom (knobH);
    auto rightKnobs = right.removeFromBottom (knobH);
    left .removeFromBottom (UIConst::knobGap);
    right.removeFromBottom (UIConst::knobGap);

    // Combo row under the waveform: [ wave combo | OCTAVE | octave combo ]
    auto comboRow = left.removeFromBottom (UIConst::buttonH);
    left.removeFromBottom (UIConst::knobGap);

    if (selectedTab == kSubTab)
    {
        // SUB: sine across the full width, then [ OCTAVE combo | SKIP FILTER ], then LEVEL
        waveformDisplay.setBounds (left.getUnion (right.withTrimmedBottom (0)).withBottom (left.getBottom()));
        auto row = comboRow;
        subOctLabel.setBounds (row.removeFromLeft (64));
        subOctCombo.setBounds (row.removeFromLeft (90));
        row.removeFromLeft (16);
        skipFilterBtn.setBounds (row.removeFromLeft (130));
        BassLookAndFeel::placeKnob (subLevelSlider, subLevelLabel, leftKnobs.removeFromLeft (leftKnobs.getWidth() / 4));
        return;
    }

    waveformDisplay.setBounds (left);
    auto octCombo = comboRow.removeFromRight (80);
    octCombo1.setBounds (octCombo);
    octCombo2.setBounds (octCombo);
    octLabel.setBounds (comboRow.removeFromRight (70));
    comboRow.removeFromRight (4);
    waveCombo1.setBounds (comboRow);
    waveCombo2.setBounds (comboRow);

    // Unison display, its caption at the bottom of the column, level with the combo row
    unisonCaption.setBounds (right.removeFromBottom (UIConst::captionH));
    right.removeFromBottom (UIConst::knobGap);
    unisonDisplay.setBounds (right);

    // The knob holders cover the whole body so their rows can be placed in two columns
    const auto whole = getBodyBounds();
    for (auto* osc : { &osc1, &osc2Comp })
    {
        osc->setBounds (whole);
        osc->layoutKnobs (leftKnobs.translated (-whole.getX(), -whole.getY()),
                          rightKnobs.translated (-whole.getX(), -whole.getY()));
    }
}
