#include "PluginEditor.h"

//==============================================================================
BassSynthAudioProcessorEditor::BassSynthAudioProcessorEditor (BassSynthAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      levelAttach    (p.apvts, "level",     levelSlider),
      mixAttach      (p.apvts, "mix",       mixSlider),
      waveformAttach (p.apvts, "waveform",  waveformBox)
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

    setupSlider (levelSlider,     levelLabel,     "Level");
    setupSlider (mixSlider,       mixLabel,       "Mix");

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

    setSize (340, 240);
}

BassSynthAudioProcessorEditor::~BassSynthAudioProcessorEditor() {}

//==============================================================================
void BassSynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1a1a2e));

    g.setColour (juce::Colour (0xffe94560));
    g.setFont (juce::Font (18.0f, juce::Font::bold));
    g.drawFittedText ("Bass Synth", getLocalBounds().removeFromTop (40),
                      juce::Justification::centred, 1);
}

void BassSynthAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (10);
    area.removeFromTop (40); // title

    // Row 1: waveform selector
    auto waveRow = area.removeFromTop (30);
    waveformLabel.setBounds (waveRow.removeFromLeft (70));
    waveformBox  .setBounds (waveRow.removeFromLeft (130));
    waveRow.removeFromLeft (8);
    loadWavButton.setBounds (waveRow.removeFromLeft (100));

    area.removeFromTop (8);

    // Row 2: labels + knobs
    const int knobWidth = area.getWidth() / 3;
    auto labelArea = area.removeFromBottom (24);
    levelLabel.setBounds (labelArea.removeFromLeft (knobWidth));
    mixLabel  .setBounds (labelArea.removeFromLeft (knobWidth));
    auto knobArea = area;
    levelSlider.setBounds (knobArea.removeFromLeft (knobWidth));
    mixSlider  .setBounds (knobArea.removeFromLeft (knobWidth));
}
