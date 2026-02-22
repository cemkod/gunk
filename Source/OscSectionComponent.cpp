#include "OscSectionComponent.h"
#include "LookAndFeel.h"

OscSectionComponent::OscSectionComponent (JQGunkAudioProcessor& p,
                                           juce::AudioProcessorValueTreeState& avts)
    : processor (p),
      apvts (avts),
      mixAttach          (avts, "mix",           mixSlider),
      subLevelAttach     (avts, "subLevel",      subLevelSlider),
      unisonVoicesAttach (avts, "unisonVoices",  unisonVoicesSlider),
      unisonDetuneAttach (avts, "unisonDetune",  unisonDetuneSlider),
      unisonBlendAttach  (avts, "unisonBlend",   unisonBlendSlider)
{
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

    setupSlider (mixSlider,      mixLabel,      "MIX");
    setupSlider (subLevelSlider, subLevelLabel, "SUB");

    setupSlider (unisonVoicesSlider, unisonVoicesLabel, "VOICES");
    unisonVoicesSlider.setNumDecimalPlacesToDisplay (0);
    setupSlider (unisonDetuneSlider, unisonDetuneLabel, "DETUNE");
    setupSlider (unisonBlendSlider,  unisonBlendLabel,  "BLEND");

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

    applyIcons (waveBtnTri,    triPath);
    applyIcons (waveBtnSq,     sqPath);
    applyIcons (waveBtnSaw,    sawPath);
    applyIcons (waveBtnCustom, customPath);

    //==========================================================================
    // Button setup
    //==========================================================================
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

    updateButtonStates();
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
}

void OscSectionComponent::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds();
    g.setColour (BassLookAndFeel::surface);
    g.fillRoundedRectangle (bounds.toFloat(), 6.0f);
    g.setColour (BassLookAndFeel::border);
    g.drawRoundedRectangle (bounds.toFloat(), 6.0f, 1.0f);
    g.setColour (BassLookAndFeel::text);
    g.setFont (juce::Font (11.0f, juce::Font::bold));
    g.drawText ("OSC", bounds.reduced (6, 4).removeFromTop (14), juce::Justification::topLeft);
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

    // Row 1: Mix | Sub knobs
    const int knobW2 = inner.getWidth() / 2;
    auto knobRow = inner.removeFromTop (75);
    mixSlider     .setBounds (knobRow.removeFromLeft (knobW2));
    subLevelSlider.setBounds (knobRow);

    auto lblRow = inner.removeFromTop (18);
    mixLabel     .setBounds (lblRow.removeFromLeft (knobW2));
    subLevelLabel.setBounds (lblRow);

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
