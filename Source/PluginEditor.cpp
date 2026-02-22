#include "PluginEditor.h"

//==============================================================================
BassSynthAudioProcessorEditor::BassSynthAudioProcessorEditor (BassSynthAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      levelAttach       (p.apvts, "level",            levelSlider),
      mixAttach         (p.apvts, "mix",               mixSlider),
      sensitivityAttach (p.apvts, "envSensitivity",    sensitivitySlider),
      resonanceAttach   (p.apvts, "envResonance",      resonanceSlider),
      decayAttach       (p.apvts, "envDecay",          decaySlider)
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

    sweepBox.addItem ("Off",  1);
    sweepBox.addItem ("Up",   2);
    sweepBox.addItem ("Down", 3);
    addAndMakeVisible (sweepBox);
    sweepAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        p.apvts, "sweepMode", sweepBox);

    setupSlider (levelSlider,       levelLabel,       "Level");
    setupSlider (mixSlider,         mixLabel,         "Mix");
    setupSlider (sensitivitySlider, sensitivityLabel, "Sensitivity");
    setupSlider (resonanceSlider,   resonanceLabel,   "Resonance");
    setupSlider (decaySlider,       decayLabel,       "Decay");

    //==========================================================================
    // Build waveform drawable icons
    //==========================================================================
    const juce::Colour dimColour    { 0xff555577 };
    const juce::Colour brightColour { 0xffe94560 };

    auto makeIcon = [&](const juce::Path& path, juce::Colour colour)
        -> std::unique_ptr<juce::DrawablePath>
    {
        auto dp = std::make_unique<juce::DrawablePath>();
        dp->setPath (path);
        dp->setFill (juce::FillType (juce::Colours::transparentBlack));
        dp->setStrokeFill (juce::FillType (colour));
        dp->setStrokeType (juce::PathStrokeType (2.0f));
        return dp;
    };

    // Sine — 4-segment cubic Bezier approximating one full sine cycle (0–100 coords)
    juce::Path sinePath;
    sinePath.startNewSubPath (0.0f, 50.0f);
    sinePath.cubicTo ( 0.0f, 28.0f, 11.0f, 10.0f, 25.0f, 10.0f);
    sinePath.cubicTo (39.0f, 10.0f, 50.0f, 28.0f, 50.0f, 50.0f);
    sinePath.cubicTo (50.0f, 72.0f, 61.0f, 90.0f, 75.0f, 90.0f);
    sinePath.cubicTo (89.0f, 90.0f, 100.0f, 72.0f, 100.0f, 50.0f);

    // Triangle
    juce::Path triPath;
    triPath.startNewSubPath (  0.0f, 50.0f);
    triPath.lineTo           ( 25.0f, 10.0f);
    triPath.lineTo           ( 75.0f, 90.0f);
    triPath.lineTo           (100.0f, 50.0f);

    // Square
    juce::Path sqPath;
    sqPath.startNewSubPath (  0.0f, 50.0f);
    sqPath.lineTo          (  0.0f, 10.0f);
    sqPath.lineTo          ( 50.0f, 10.0f);
    sqPath.lineTo          ( 50.0f, 90.0f);
    sqPath.lineTo          (100.0f, 90.0f);
    sqPath.lineTo          (100.0f, 50.0f);

    // Sawtooth
    juce::Path sawPath;
    sawPath.startNewSubPath (  0.0f, 90.0f);
    sawPath.lineTo           ( 50.0f, 10.0f);
    sawPath.lineTo           ( 50.0f, 90.0f);
    sawPath.lineTo           (100.0f, 10.0f);

    // Custom — three vertical bars representing audio waveform
    juce::Path customPath;
    customPath.startNewSubPath (20.0f, 30.0f); customPath.lineTo (20.0f, 70.0f);
    customPath.startNewSubPath (50.0f, 15.0f); customPath.lineTo (50.0f, 85.0f);
    customPath.startNewSubPath (80.0f, 35.0f); customPath.lineTo (80.0f, 65.0f);

    // Apply icons to buttons
    auto applyIcons = [&](juce::DrawableButton& btn, const juce::Path& path)
    {
        auto dim    = makeIcon (path, dimColour);
        auto bright = makeIcon (path, brightColour);
        btn.setImages (dim.get(), nullptr, nullptr, nullptr,
                       bright.get(), nullptr, nullptr, nullptr);
    };

    applyIcons (waveBtnSine,   sinePath);
    applyIcons (waveBtnTri,    triPath);
    applyIcons (waveBtnSq,     sqPath);
    applyIcons (waveBtnSaw,    sawPath);
    applyIcons (waveBtnCustom, customPath);

    //==========================================================================
    // Button setup
    //==========================================================================
    const int waveGroup = 101;
    for (auto* b : { &waveBtnSine, &waveBtnTri, &waveBtnSq, &waveBtnSaw })
    {
        b->setRadioGroupId (waveGroup);
        b->setClickingTogglesState (true);
        addAndMakeVisible (b);
    }
    addAndMakeVisible (waveBtnCustom);

    waveBtnSine.onClick = [this] { setWaveformParam (0); };
    waveBtnTri .onClick = [this] { setWaveformParam (1); };
    waveBtnSq  .onClick = [this] { setWaveformParam (2); };
    waveBtnSaw .onClick = [this] { setWaveformParam (3); };

    waveBtnCustom.onClick = [this]
    {
        if (! processor.isCustomWavetableLoaded())
        {
            openWavFileDialog();
        }
        else if (! processor.isCustomWaveformActive())
        {
            processor.reactivateCustomWavetable();
        }
        else
        {
            openWavFileDialog(); // reload
        }
    };

    updateWaveButtonStates();
    startTimerHz (30);

    setSize (340, 390);
}

BassSynthAudioProcessorEditor::~BassSynthAudioProcessorEditor()
{
    stopTimer();
}

//==============================================================================
void BassSynthAudioProcessorEditor::setWaveformParam (int idx)
{
    auto* p = dynamic_cast<juce::AudioParameterChoice*> (
        processor.apvts.getParameter ("waveform"));
    if (p) *p = idx;
}

void BassSynthAudioProcessorEditor::openWavFileDialog()
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
            else
                updateWaveButtonStates();
        });
}

void BassSynthAudioProcessorEditor::timerCallback()
{
    updateWaveButtonStates();
}

void BassSynthAudioProcessorEditor::updateWaveButtonStates()
{
    const int idx = (int) processor.apvts.getRawParameterValue ("waveform")->load();
    const bool customActive = processor.isCustomWaveformActive();

    waveBtnSine  .setToggleState (!customActive && idx == 0, juce::dontSendNotification);
    waveBtnTri   .setToggleState (!customActive && idx == 1, juce::dontSendNotification);
    waveBtnSq    .setToggleState (!customActive && idx == 2, juce::dontSendNotification);
    waveBtnSaw   .setToggleState (!customActive && idx == 3, juce::dontSendNotification);
    waveBtnCustom.setToggleState (customActive,              juce::dontSendNotification);
}

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
    oscSectionRect = area.removeFromTop (155);
    {
        auto inner = oscSectionRect.reduced (8);
        inner.removeFromTop (18); // skip section label row

        // Waveform button row
        auto waveRow = inner.removeFromTop (36);
        const int btnW = waveRow.getWidth() / 5;
        waveBtnSine  .setBounds (waveRow.removeFromLeft (btnW));
        waveBtnTri   .setBounds (waveRow.removeFromLeft (btnW));
        waveBtnSq    .setBounds (waveRow.removeFromLeft (btnW));
        waveBtnSaw   .setBounds (waveRow.removeFromLeft (btnW));
        waveBtnCustom.setBounds (waveRow); // remainder

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
    filterSectionRect = area.removeFromTop (153);
    {
        auto inner = filterSectionRect.reduced (8);
        inner.removeFromTop (18); // skip section label row

        // Sweep row
        auto sweepRow = inner.removeFromTop (24);
        auto sweepLabel = sweepRow.removeFromLeft (50);
        juce::ignoreUnused (sweepLabel);
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
