#include "OscSectionComponent.h"
#include "LookAndFeel.h"

OscSectionComponent::OscSectionComponent (JQGunkAudioProcessor& p,
                                           juce::AudioProcessorValueTreeState& avts)
    : LabelledSectionComponent ("OSC"),
      processor (p),
      apvts (avts),
      mixAttach          (avts, "mix",           mixSlider),
      subLevelAttach     (avts, "subLevel",      subLevelSlider),
      unisonVoicesAttach (avts, "unisonVoices",  unisonVoicesSlider),
      unisonDetuneAttach (avts, "unisonDetune",  unisonDetuneSlider),
      unisonBlendAttach  (avts, "unisonBlend",   unisonBlendSlider)
{
    BassLookAndFeel::setupRotarySlider (mixSlider,      mixLabel,      "MIX",    *this);
    BassLookAndFeel::setupRotarySlider (subLevelSlider, subLevelLabel, "SUB",    *this);

    BassLookAndFeel::setupRotarySlider (unisonVoicesSlider, unisonVoicesLabel, "VOICES", *this);
    unisonVoicesSlider.setNumDecimalPlacesToDisplay (0);
    BassLookAndFeel::setupRotarySlider (unisonDetuneSlider, unisonDetuneLabel, "DETUNE", *this);
    BassLookAndFeel::setupRotarySlider (unisonBlendSlider,  unisonBlendLabel,  "BLEND",  *this);

    buildWaveformIcons();
    configureWaveformButtons();
    configureOctaveButtons();

    updateButtonStates();
}

void OscSectionComponent::buildWaveformIcons()
{
    auto makeIcon = [](const juce::Path& path, juce::Colour colour)
        -> std::unique_ptr<juce::DrawablePath>
    {
        auto dp = std::make_unique<juce::DrawablePath>();
        dp->setPath (path);
        dp->setFill (juce::FillType (juce::Colours::transparentBlack));
        dp->setStrokeFill (juce::FillType (colour));
        dp->setStrokeType (juce::PathStrokeType (13.0f));
        return dp;
    };

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

    auto applyIcons = [&](juce::DrawableButton& btn, const juce::Path& path)
    {
        auto dim    = makeIcon (path, BassLookAndFeel::iconDim);
        auto bright = makeIcon (path, BassLookAndFeel::accent);
        btn.setImages (dim.get(), nullptr, nullptr, nullptr,
                       bright.get(), nullptr, nullptr, nullptr);
    };

    applyIcons (waveBtnTri,    triPath);
    applyIcons (waveBtnSq,     sqPath);
    applyIcons (waveBtnSaw,    sawPath);
    applyIcons (waveBtnCustom, customPath);
}

void OscSectionComponent::configureOctaveButtons()
{
    octLabel.setText ("OCTAVE", juce::dontSendNotification);
    octLabel.setFont (juce::Font (UIConst::uiFontSize));
    octLabel.setColour (juce::Label::textColourId, BassLookAndFeel::textDim);
    octLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (octLabel);

    const int octGroup = 102;
    for (auto* b : { &octBtn0, &octBtn1, &octBtn2 })
    {
        b->setRadioGroupId (octGroup);
        b->setClickingTogglesState (true);
        b->setColour (juce::TextButton::buttonColourId,     BassLookAndFeel::surface);
        b->setColour (juce::TextButton::buttonOnColourId,   BassLookAndFeel::accent);
        b->setColour (juce::TextButton::textColourOffId,    BassLookAndFeel::iconDim);
        b->setColour (juce::TextButton::textColourOnId,     juce::Colours::black);
        addAndMakeVisible (b);
    }

    auto setOctParam = [this](int idx)
    {
        auto* p = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter ("octaveShift"));
        if (p) *p = idx;
    };

    octBtn0.onClick = [setOctParam] { setOctParam (0); };
    octBtn1.onClick = [setOctParam] { setOctParam (1); };
    octBtn2.onClick = [setOctParam] { setOctParam (2); };
}

void OscSectionComponent::configureWaveformButtons()
{
    const int waveGroup = 101;
    for (auto* b : { &waveBtnTri, &waveBtnSq, &waveBtnSaw })
    {
        b->setRadioGroupId (waveGroup);
        b->setClickingTogglesState (true);
        addAndMakeVisible (b);
    }
    addAndMakeVisible (waveBtnCustom);

    waveBtnTri .onClick = [this] { setWaveformParam (0); };
    waveBtnSq  .onClick = [this] { setWaveformParam (1); };
    waveBtnSaw .onClick = [this] { setWaveformParam (2); };

    waveBtnCustom.onClick = [this]
    {
        if (! processor.isCustomWavetableLoaded())
            openWavFileDialog();
        else if (! processor.isCustomWaveformActive())
            processor.reactivateCustomWavetable();
        else
            openWavFileDialog(); // reload
    };
}

OscSectionComponent::~OscSectionComponent()
{
    setLookAndFeel (nullptr);
}

void OscSectionComponent::setWaveformParam (int idx)
{
    auto* p = dynamic_cast<juce::AudioParameterChoice*> (
        apvts.getParameter ("waveform"));
    if (p) *p = idx;
}

void OscSectionComponent::openWavFileDialog()
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
                updateButtonStates();
        });
}

void OscSectionComponent::updateButtonStates()
{
    const int idx = (int) apvts.getRawParameterValue ("waveform")->load();
    const bool customActive = processor.isCustomWaveformActive();

    waveBtnTri   .setToggleState (!customActive && idx == 0, juce::dontSendNotification);
    waveBtnSq    .setToggleState (!customActive && idx == 1, juce::dontSendNotification);
    waveBtnSaw   .setToggleState (!customActive && idx == 2, juce::dontSendNotification);
    waveBtnCustom.setToggleState (customActive,              juce::dontSendNotification);

    const int octIdx = (int) apvts.getRawParameterValue ("octaveShift")->load();
    octBtn0.setToggleState (octIdx == 0, juce::dontSendNotification);
    octBtn1.setToggleState (octIdx == 1, juce::dontSendNotification);
    octBtn2.setToggleState (octIdx == 2, juce::dontSendNotification);
}


void OscSectionComponent::resized()
{
    auto inner = getLocalBounds().reduced (8);
    inner.removeFromTop (18); // skip section label row

    // Waveform button row
    auto waveRow = inner.removeFromTop (36);
    const int btnW = waveRow.getWidth() / 4;
    waveBtnTri   .setBounds (waveRow.removeFromLeft (btnW));
    waveBtnSq    .setBounds (waveRow.removeFromLeft (btnW));
    waveBtnSaw   .setBounds (waveRow.removeFromLeft (btnW));
    waveBtnCustom.setBounds (waveRow); // remainder

    inner.removeFromTop (10);

    // Row 1: Mix | Octave gang | Sub
    // Three equal columns; octave gang is label + 3 stacked buttons
    const int colW = inner.getWidth() / 3;
    auto knobRow = inner.removeFromTop (75);
    auto lblRow  = inner.removeFromTop (18);

    mixSlider     .setBounds (knobRow.removeFromLeft (colW));
    mixLabel      .setBounds (lblRow .removeFromLeft (colW));

    // Middle column: octave label at top, then 3 stacked buttons
    auto octCol = knobRow.removeFromLeft (colW);
    lblRow.removeFromLeft (colW); // consume middle label slot (octave label is inside knobRow)

    octLabel.setBounds (octCol.removeFromTop (14));
    const int btnH = octCol.getHeight() / 3;
    octBtn0.setBounds (octCol.removeFromTop (btnH));
    octBtn1.setBounds (octCol.removeFromTop (btnH));
    octBtn2.setBounds (octCol);

    subLevelSlider.setBounds (knobRow); // remainder
    subLevelLabel .setBounds (lblRow);

    inner.removeFromTop (8);

    // Row 2: Voices | Detune | Blend knobs
    const int knobW3 = inner.getWidth() / 3;
    auto knobRow2 = inner.removeFromTop (75);
    unisonVoicesSlider.setBounds (knobRow2.removeFromLeft (knobW3));
    unisonDetuneSlider.setBounds (knobRow2.removeFromLeft (knobW3));
    unisonBlendSlider .setBounds (knobRow2);

    auto lblRow2 = inner.removeFromTop (18);
    unisonVoicesLabel.setBounds (lblRow2.removeFromLeft (knobW3));
    unisonDetuneLabel.setBounds (lblRow2.removeFromLeft (knobW3));
    unisonBlendLabel .setBounds (lblRow2);
}
