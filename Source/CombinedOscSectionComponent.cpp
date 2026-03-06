#include "CombinedOscSectionComponent.h"
#include "LookAndFeel.h"

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
    : LabelledSectionComponent ("OSC"),
      apvts (avts),
      osc1    ("", ids1, avts, true),
      osc2Comp("", ids2, avts, true),
      octAttach1 (avts, ids1.octaveShift, octCombo1),
      octAttach2 (avts, ids2.octaveShift, octCombo2)
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

    // Tab buttons
    const int tabGroup = juce::Random::getSystemRandom().nextInt (0x7fffffff);
    for (auto* b : { &tabBtn1, &tabBtn2 })
    {
        b->setRadioGroupId (tabGroup);
        b->setClickingTogglesState (true);
        b->setColour (juce::TextButton::buttonColourId,   BassLookAndFeel::surface);
        b->setColour (juce::TextButton::buttonOnColourId, BassLookAndFeel::accent);
        b->setColour (juce::TextButton::textColourOffId,  BassLookAndFeel::iconDim);
        b->setColour (juce::TextButton::textColourOnId,   juce::Colours::black);
        addAndMakeVisible (b);
    }
    tabBtn1.onClick = [this] { selectOsc (0); };
    tabBtn2.onClick = [this] { selectOsc (1); };
    tabBtn1.setToggleState (true, juce::dontSendNotification);

    // Waveform display + unison display
    addAndMakeVisible (waveformDisplay);
    addAndMakeVisible (unisonDisplay);
    selectOsc (0); // initialises unisonDisplay callbacks

    // Waveform combos
    setupWaveCombo (waveCombo1, 0);
    setupWaveCombo (waveCombo2, 1);
    waveCombo2.setVisible (false);

    // Octave combos
    auto setupOct = [this] (juce::ComboBox& combo)
    {
        combo.addItem ("-2", 1);
        combo.addItem ("-1", 2);
        combo.addItem ( "0", 3);
        combo.addItem ("+1", 4);
        combo.addItem ("+2", 5);
        addAndMakeVisible (combo);
    };
    setupOct (octCombo1);
    setupOct (octCombo2);
    octCombo2.setVisible (false);

    octLabel.setText ("OCT", juce::dontSendNotification);
    octLabel.setFont (juce::Font (UIConst::uiFontSize, juce::Font::bold));
    octLabel.setColour (juce::Label::textColourId, BassLookAndFeel::textDim);
    octLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (octLabel);

    addAndMakeVisible (osc1);
    addAndMakeVisible (osc2Comp);
    osc2Comp.setVisible (false);
}

CombinedOscSectionComponent::~CombinedOscSectionComponent()
{
    setLookAndFeel (nullptr);
}

void CombinedOscSectionComponent::selectOsc (int idx)
{
    selectedOsc = idx;
    osc1    .setVisible (idx == 0);
    osc2Comp.setVisible (idx == 1);

    tabBtn1.setToggleState (idx == 0, juce::dontSendNotification);
    tabBtn2.setToggleState (idx == 1, juce::dontSendNotification);

    waveformDisplay.getDisplayFrame = getDisplayFrame[(size_t) idx];
    waveformDisplay.repaint();

    unisonDisplay.getVoices = [this, idx] {
        auto* p = apvts.getParameter (paramIds[idx].unisonVoices);
        return p ? p->getValue() * 7.0f + 1.0f : 1.0f;
    };
    unisonDisplay.getDetune = [this, idx] {
        return getModDetune ? getModDetune (idx) : 0.0f;
    };
    unisonDisplay.getBlend = [this, idx] {
        return getModBlend ? getModBlend (idx) : 1.0f;
    };

    waveCombo1.setVisible (idx == 0);
    waveCombo2.setVisible (idx == 1);
    octCombo1.setVisible (idx == 0);
    octCombo2.setVisible (idx == 1);
    syncWaveCombo (idx);
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
        // id 50 = loaded custom wavetable — no action needed, already active
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

    // Sync to APVTS waveform param (0=Triangle,1=Square,2=Sawtooth → IDs 1,2,3)
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
    waveformDisplay.getDisplayFrame = getDisplayFrame[(size_t) selectedOsc];

    syncWaveCombo (0);
    syncWaveCombo (1);

    if (selectedOsc == 0)
        osc1.updateButtonStates();
    else
        osc2Comp.updateButtonStates();
}

void CombinedOscSectionComponent::resized()
{
    auto inner = getLocalBounds().reduced (UIConst::sectionInnerPad);
    inner.removeFromTop (UIConst::sectionHeaderH); // section label row

    // Tab buttons sit in the header bar (not consuming inner space)
    {
        const int btnW = 18;
        const int btnH = 13;
        const int pad  = UIConst::sectionInnerPad;
        auto header = getLocalBounds().removeFromTop (UIConst::sectionHeaderH);
        auto btnArea = header.removeFromRight (pad + btnW * 2)
                             .withSizeKeepingCentre (btnW * 2, btnH);
        tabBtn1.setBounds (btnArea.removeFromLeft (btnW));
        tabBtn2.setBounds (btnArea);
    }

    // Waveform display (left half) + placeholder (right half)
    auto displayRow = inner.removeFromTop (UIConst::displayH_waveform);
    const int gap = 4;
    waveformDisplay  .setBounds (displayRow.removeFromLeft (displayRow.getWidth() / 2 - gap / 2));
    displayRow.removeFromLeft (gap);
    unisonDisplay.setBounds (displayRow);

    inner.removeFromTop (4);

    // Waveform selector + octave selector (side by side)
    auto comboRow = inner.removeFromTop (UIConst::buttonH);
    const int octW   = 46;
    const int octLblW = 28;
    auto octComboArea = comboRow.removeFromRight (octW);
    auto octLblArea   = comboRow.removeFromRight (octLblW);
    octLabel  .setBounds (octLblArea);
    octCombo1 .setBounds (octComboArea);
    octCombo2 .setBounds (octComboArea);
    waveCombo1.setBounds (comboRow);
    waveCombo2.setBounds (comboRow);

    inner.removeFromTop (UIConst::knobGap);

    // Remaining: osc controls (both at same bounds, one visible)
    osc1    .setBounds (inner);
    osc2Comp.setBounds (inner);
}
