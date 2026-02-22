#include "PluginEditor.h"
#include <BinaryData.h>

//==============================================================================
JQGunkAudioProcessorEditor::JQGunkAudioProcessorEditor (JQGunkAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      levelAttach       (p.apvts, "level",            levelSlider),
      mixAttach         (p.apvts, "mix",               mixSlider),
      sensitivityAttach (p.apvts, "envSensitivity",    sensitivitySlider),
      resonanceAttach   (p.apvts, "envResonance",      resonanceSlider),
      decayAttach          (p.apvts, "envDecay",          decaySlider),
      gateThresholdAttach  (p.apvts, "gateThreshold",    gateThresholdSlider),
      gateHysteresisAttach (p.apvts, "gateHysteresis",   gateHysteresisSlider)
{
    setLookAndFeel (&lookAndFeel);

    auto setupSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& name)
    {
        s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
        s.setColour (juce::Slider::textBoxTextColourId,       BassLookAndFeel::text);
        s.setColour (juce::Slider::textBoxBackgroundColourId, BassLookAndFeel::surfaceDark);
        s.setColour (juce::Slider::textBoxOutlineColourId,    BassLookAndFeel::borderDim);
        addAndMakeVisible (s);

        l.setText (name, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.setFont (juce::Font (11.0f, juce::Font::bold));
        l.setColour (juce::Label::textColourId, BassLookAndFeel::text);
        addAndMakeVisible (l);
    };

    // Sweep buttons
    const int sweepGroup = 102;
    for (auto* b : { &sweepBtnOff, &sweepBtnUp, &sweepBtnDown })
    {
        b->setRadioGroupId (sweepGroup);
        b->setClickingTogglesState (true);
        addAndMakeVisible (b);
    }
    sweepBtnOff .onClick = [this] { setSweepParam (0); };
    sweepBtnUp  .onClick = [this] { setSweepParam (1); };
    sweepBtnDown.onClick = [this] { setSweepParam (2); };

    setupSlider (levelSlider,       levelLabel,       "LEVEL");
    setupSlider (mixSlider,         mixLabel,         "MIX");
    setupSlider (sensitivitySlider,   sensitivityLabel,   "SENS");
    setupSlider (resonanceSlider,     resonanceLabel,     "RESO");
    setupSlider (decaySlider,         decayLabel,         "DECAY");
    setupSlider (gateThresholdSlider,  gateThresholdLabel,  "THRS");
    setupSlider (gateHysteresisSlider, gateHysteresisLabel, "HYST");

    //==========================================================================
    // Build waveform drawable icons
    //==========================================================================
    const juce::Colour dimColour    = BassLookAndFeel::iconDim;
    const juce::Colour brightColour = BassLookAndFeel::accent;

    auto makeIcon = [&](const juce::Path& path, juce::Colour colour)
        -> std::unique_ptr<juce::DrawablePath>
    {
        auto dp = std::make_unique<juce::DrawablePath>();
        dp->setPath (path);
        dp->setFill (juce::FillType (juce::Colours::transparentBlack));
        dp->setStrokeFill (juce::FillType (colour));
        dp->setStrokeType (juce::PathStrokeType (13.0f));
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
    sawPath.lineTo           ( 70.0f, 10.0f);
    sawPath.lineTo           ( 70.0f, 90.0f);
    sawPath.lineTo           (140.0f, 10.0f);

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
    updateSweepButtonStates();

    freqLabel.setFont (juce::Font (11.0f));
    freqLabel.setColour (juce::Label::textColourId, BassLookAndFeel::text.withAlpha (0.7f));
    freqLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (freqLabel);

    startTimerHz (30);

    if (auto svgXml = juce::parseXML (juce::String::fromUTF8 (BinaryData::logo_svg, BinaryData::logo_svgSize)))
        logo = juce::Drawable::createFromSVG (*svgXml);

    setSize (340, 571);
}

JQGunkAudioProcessorEditor::~JQGunkAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
    stopTimer();
}

//==============================================================================
void JQGunkAudioProcessorEditor::setWaveformParam (int idx)
{
    auto* p = dynamic_cast<juce::AudioParameterChoice*> (
        processor.apvts.getParameter ("waveform"));
    if (p) *p = idx;
}

void JQGunkAudioProcessorEditor::setSweepParam (int idx)
{
    auto* p = dynamic_cast<juce::AudioParameterChoice*> (
        processor.apvts.getParameter ("sweepMode"));
    if (p) *p = idx;
}

void JQGunkAudioProcessorEditor::openWavFileDialog()
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

void JQGunkAudioProcessorEditor::timerCallback()
{
    updateWaveButtonStates();
    updateSweepButtonStates();

    const float freq = processor.getDetectedFrequency();
    if (freq > 0.0f)
        freqLabel.setText (juce::String (freq, 1) + " Hz", juce::dontSendNotification);
    else
        freqLabel.setText ("-- Hz", juce::dontSendNotification);

    const bool nowOpen = processor.isGateOpen();
    if (nowOpen != gateOpen)
    {
        gateOpen = nowOpen;
        repaint (gateLedBounds);
    }
}

void JQGunkAudioProcessorEditor::updateWaveButtonStates()
{
    const int idx = (int) processor.apvts.getRawParameterValue ("waveform")->load();
    const bool customActive = processor.isCustomWaveformActive();

    waveBtnSine  .setToggleState (!customActive && idx == 0, juce::dontSendNotification);
    waveBtnTri   .setToggleState (!customActive && idx == 1, juce::dontSendNotification);
    waveBtnSq    .setToggleState (!customActive && idx == 2, juce::dontSendNotification);
    waveBtnSaw   .setToggleState (!customActive && idx == 3, juce::dontSendNotification);
    waveBtnCustom.setToggleState (customActive,              juce::dontSendNotification);
}

void JQGunkAudioProcessorEditor::updateSweepButtonStates()
{
    const int idx = (int) processor.apvts.getRawParameterValue ("sweepMode")->load();
    sweepBtnOff .setToggleState (idx == 0, juce::dontSendNotification);
    sweepBtnUp  .setToggleState (idx == 1, juce::dontSendNotification);
    sweepBtnDown.setToggleState (idx == 2, juce::dontSendNotification);
}

//==============================================================================
void JQGunkAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (BassLookAndFeel::bg);

    // Section backgrounds
    g.setColour (BassLookAndFeel::surface);
    g.fillRoundedRectangle (gateSectionRect.toFloat(),   6.0f);
    g.fillRoundedRectangle (oscSectionRect.toFloat(),    6.0f);
    g.fillRoundedRectangle (filterSectionRect.toFloat(), 6.0f);

    // Section borders
    g.setColour (BassLookAndFeel::border);
    g.drawRoundedRectangle (gateSectionRect.toFloat(),   6.0f, 1.0f);
    g.drawRoundedRectangle (oscSectionRect.toFloat(),    6.0f, 1.0f);
    g.drawRoundedRectangle (filterSectionRect.toFloat(), 6.0f, 1.0f);

    // Section labels
    g.setColour (BassLookAndFeel::text);
    g.setFont (juce::Font (11.0f, juce::Font::bold));
    g.drawText ("GATE",       gateSectionRect.reduced (6, 4).removeFromTop (14), juce::Justification::topLeft);
    g.drawText ("OSC",        oscSectionRect.reduced (6, 4).removeFromTop (14), juce::Justification::topLeft);
    g.drawText ("ENV FILTER", filterSectionRect.reduced (6, 4).removeFromTop (14), juce::Justification::topLeft);

    if (logo != nullptr)
    {
        constexpr float logoH = 140.0f;
        constexpr float logoW = logoH * (184.0f / 98.0f);
        auto headerBounds = getLocalBounds().removeFromTop (60).toFloat();
        juce::Rectangle<float> logoBounds (
            (headerBounds.getWidth() - logoW) * 0.5f,
            (headerBounds.getHeight() - logoH) * 0.6f,
            logoW, logoH);
        logo->drawWithin (g, logoBounds, juce::RectanglePlacement::centred, 1.0f);
    }

    // Gate LED
    {
        const auto bounds = gateLedBounds.toFloat();
        const juce::Colour ledOn  { 0xff44ff88 };
        const juce::Colour ledOff { 0xff223322 };
        const juce::Colour col = gateOpen ? ledOn : ledOff;

        // Outer glow when open
        if (gateOpen)
        {
            g.setColour (ledOn.withAlpha (0.25f));
            g.fillEllipse (bounds.expanded (3.0f));
        }

        g.setColour (col);
        g.fillEllipse (bounds);

        // Highlight specular
        g.setColour (juce::Colours::white.withAlpha (gateOpen ? 0.35f : 0.08f));
        g.fillEllipse (bounds.reduced (1.5f).withHeight (bounds.getHeight() * 0.45f));
    }
}

void JQGunkAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (10);
    auto header = area.removeFromTop (60);
    auto labelRow = header.removeFromBottom (16);
    const int ledSize = 8;
    gateLedBounds = labelRow.removeFromLeft (ledSize + 6)
                            .withSizeKeepingCentre (ledSize, ledSize);
    freqLabel.setBounds (labelRow);
    area.removeFromTop (4);  // gap below title

    // --- Gate section ---
    gateSectionRect = area.removeFromTop (137);
    {
        auto inner = gateSectionRect.reduced (8);
        inner.removeFromTop (18); // skip section label row
        inner.removeFromTop (10);
        const int knobW = inner.getWidth() / 2;
        auto knobRow = inner.removeFromTop (75);
        gateThresholdSlider .setBounds (knobRow.removeFromLeft (knobW));
        gateHysteresisSlider.setBounds (knobRow.removeFromLeft (knobW));
        auto lblRow = inner.removeFromTop (18);
        gateThresholdLabel .setBounds (lblRow.removeFromLeft (knobW));
        gateHysteresisLabel.setBounds (lblRow.removeFromLeft (knobW));
    }
    area.removeFromTop (8); // gap before OSC

    // --- Oscillator section ---
    oscSectionRect = area.removeFromTop (173);
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

        inner.removeFromTop (10);

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
    filterSectionRect = area.removeFromTop (161);
    {
        auto inner = filterSectionRect.reduced (8);
        inner.removeFromTop (18); // skip section label row

        // Sweep button row
        auto sweepRow = inner.removeFromTop (24);
        const int btnW = sweepRow.getWidth() / 3;
        sweepBtnOff .setBounds (sweepRow.removeFromLeft (btnW));
        sweepBtnUp  .setBounds (sweepRow.removeFromLeft (btnW));
        sweepBtnDown.setBounds (sweepRow);

        inner.removeFromTop (10);

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
