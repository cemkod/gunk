#include "PluginEditor.h"

//==============================================================================
BassSynthAudioProcessorEditor::BassSynthAudioProcessorEditor (BassSynthAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      levelAttach       (p.apvts, "level",            levelSlider),
      mixAttach         (p.apvts, "mix",               mixSlider),
      sensitivityAttach (p.apvts, "envSensitivity",    sensitivitySlider),
      resonanceAttach   (p.apvts, "envResonance",      resonanceSlider),
      decayAttach       (p.apvts, "envDecay",          decaySlider),
      sweepAttach       (p.apvts, "sweepMode",          sweepBox),
      waveformAttach    (p.apvts, "waveform",          waveformBox)
{
    auto setupSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& name)
    {
        s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
        addAndMakeVisible (s);

        l.setText (name, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (l);
    };

    setupSlider (levelSlider,       levelLabel,       "Level");
    setupSlider (mixSlider,         mixLabel,         "Mix");
    setupSlider (sensitivitySlider, sensitivityLabel, "Sensitivity");
    setupSlider (resonanceSlider,   resonanceLabel,   "Resonance");
    setupSlider (decaySlider,       decayLabel,       "Decay");
    sweepBox.addItem ("Off",  1);
    sweepBox.addItem ("Up",   2);
    sweepBox.addItem ("Down", 3);
    addAndMakeVisible (sweepBox);

    // Waveform ComboBox (item IDs 1-based; ComboBoxAttachment maps param 0 → ID 1)
    waveformBox.addItem ("Sine",     1);
    waveformBox.addItem ("Triangle", 2);
    waveformBox.addItem ("Square",   3);
    waveformBox.addItem ("Sawtooth", 4);
    addAndMakeVisible (waveformBox);

    waveformLabel.setText ("Waveform", juce::dontSendNotification);
    waveformLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (waveformLabel);

    loadWavButton.onClick = [this]()
    {
        fileChooser = std::make_unique<juce::FileChooser> (
            "Select a WAV file",
            juce::File::getSpecialLocation (juce::File::userHomeDirectory),
            "*.wav");
        fileChooser->launchAsync (
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this] (const juce::FileChooser& fc)
            {
                auto results = fc.getResults();
                if (results.isEmpty()) return;
                if (! processor.loadWavetableFromFile (results.getFirst()))
                    juce::AlertWindow::showMessageBoxAsync (
                        juce::AlertWindow::WarningIcon, "Load failed",
                        "Could not read the selected WAV file.", "OK");
            });
    };
    addAndMakeVisible (loadWavButton);

    setSize (340, 390);
}

BassSynthAudioProcessorEditor::~BassSynthAudioProcessorEditor() {}

//==============================================================================
void BassSynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1a1a2e));

    // Section backgrounds
    g.setColour (juce::Colour (0xff252540));
    g.fillRoundedRectangle (oscSectionRect.toFloat(),    6.0f);
    g.fillRoundedRectangle (filterSectionRect.toFloat(), 6.0f);

    // Section labels
    g.setColour (juce::Colour (0xffe94560));
    g.setFont (juce::Font (11.0f, juce::Font::bold));
    g.drawText ("OSCILLATOR",      oscSectionRect.reduced (6, 4).removeFromTop (14), juce::Justification::topLeft);
    g.drawText ("ENVELOPE FILTER", filterSectionRect.reduced (6, 4).removeFromTop (14), juce::Justification::topLeft);

    // Title
    g.setColour (juce::Colour (0xffe94560));
    g.setFont (juce::Font (18.0f, juce::Font::bold));
    g.drawFittedText ("Bass Synth", getLocalBounds().removeFromTop (40),
                      juce::Justification::centred, 1);
}

void BassSynthAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (10);
    area.removeFromTop (40); // title
    area.removeFromTop (4);  // gap below title

    // --- Oscillator section ---
    // height: 14 label + 4 + 26 waveRow + 4 + 75 knobs + 18 lblRow + 8 pad + 16 top = 165
    oscSectionRect = area.removeFromTop (155);
    {
        auto inner = oscSectionRect.reduced (8);
        inner.removeFromTop (18); // skip section label row

        // Waveform row
        auto waveRow = inner.removeFromTop (26);
        waveformLabel.setBounds (waveRow.removeFromLeft (70));
        waveformBox  .setBounds (waveRow.removeFromLeft (130));
        waveRow.removeFromLeft (8);
        loadWavButton.setBounds (waveRow.removeFromLeft (90));

        inner.removeFromTop (4);

        // Level | Mix knobs
        const int knobW = inner.getWidth() / 2;
        auto knobRow = inner.removeFromTop (75);
        levelSlider.setBounds (knobRow.removeFromLeft (knobW));
        mixSlider  .setBounds (knobRow.removeFromLeft (knobW));

        auto lblRow = inner.removeFromTop (18);
        levelLabel.setBounds (lblRow.removeFromLeft (knobW));
        mixLabel  .setBounds (lblRow.removeFromLeft (knobW));
    }

    area.removeFromTop (8); // gap between sections

    // --- Envelope Filter section ---
    // height: 14 label + 4 + 24 sweepRow + 4 + 75 knobs + 18 lblRow + 8 pad + 10 top = 157 → use 153
    filterSectionRect = area.removeFromTop (153);
    {
        auto inner = filterSectionRect.reduced (8);
        inner.removeFromTop (18); // skip section label row

        // Sweep row
        auto sweepRow = inner.removeFromTop (24);
        auto sweepLabel = sweepRow.removeFromLeft (50);
        // (no persistent Label widget for "Sweep:" — drawn inline via paint or omitted)
        sweepBox.setBounds (sweepRow.removeFromLeft (130));

        inner.removeFromTop (4);

        // Sensitivity | Resonance | Decay knobs
        const int knobW = inner.getWidth() / 3;
        auto knobRow = inner.removeFromTop (75);
        sensitivitySlider.setBounds (knobRow.removeFromLeft (knobW));
        resonanceSlider  .setBounds (knobRow.removeFromLeft (knobW));
        decaySlider      .setBounds (knobRow.removeFromLeft (knobW));

        auto lblRow = inner.removeFromTop (18);
        sensitivityLabel.setBounds (lblRow.removeFromLeft (knobW));
        resonanceLabel  .setBounds (lblRow.removeFromLeft (knobW));
        decayLabel      .setBounds (lblRow.removeFromLeft (knobW));
    }
}
