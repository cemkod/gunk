#include "CombinedOscSectionComponent.h"
#include "LookAndFeel.h"

CombinedOscSectionComponent::CombinedOscSectionComponent (
    juce::AudioProcessorValueTreeState& avts,
    const OscParamIds& ids1,
    const OscParamIds& ids2)
    : LabelledSectionComponent ("OSC"),
      apvts (avts),
      osc1    ("", ids1, avts, true),
      osc2Comp("", ids2, avts, true)
{
    paramIds[0] = ids1;
    paramIds[1] = ids2;

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

    // Waveform display
    waveformDisplay.onMouseDown = [this] { showWaveformMenu(); };
    addAndMakeVisible (waveformDisplay);

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

    waveformDisplay.getDisplayFrame  = getDisplayFrame[(size_t) idx];
    waveformDisplay.getWavetableName = getWavetableName[(size_t) idx];
    waveformDisplay.repaint();
}

void CombinedOscSectionComponent::showWaveformMenu()
{
    const int osc = selectedOsc;
    juce::PopupMenu menu;
    menu.addItem (1, "Triangle");
    menu.addItem (2, "Square");
    menu.addItem (3, "Sawtooth");
    menu.addSeparator();
    menu.addItem (4, "Load WAV or WT file\xe2\x80\xa6");

    menu.showMenuAsync (juce::PopupMenu::Options{}.withTargetComponent (waveformDisplay),
        [this, osc] (int result)
        {
            if (result >= 1 && result <= 3)
                setWaveformParam (osc, result - 1);
            else if (result == 4)
                openWavFileDialog (osc);
        });
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
            if (! ok)
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
    waveformDisplay.getDisplayFrame  = getDisplayFrame[(size_t) selectedOsc];
    waveformDisplay.getWavetableName = getWavetableName[(size_t) selectedOsc];

    if (selectedOsc == 0)
        osc1.updateButtonStates();
    else
        osc2Comp.updateButtonStates();
}

void CombinedOscSectionComponent::resized()
{
    auto inner = getLocalBounds().reduced (8);
    inner.removeFromTop (18); // section label row

    // Tab row (20px)
    auto tabRow = inner.removeFromTop (20);
    const int tabW = 28;
    tabBtn1.setBounds (tabRow.removeFromLeft (tabW));
    tabRow.removeFromLeft (4);
    tabBtn2.setBounds (tabRow.removeFromLeft (tabW));

    inner.removeFromTop (4);

    // Waveform display (56px)
    waveformDisplay.setBounds (inner.removeFromTop (56));

    inner.removeFromTop (4);

    // Remaining: osc controls (both at same bounds, one visible)
    osc1    .setBounds (inner);
    osc2Comp.setBounds (inner);
}
