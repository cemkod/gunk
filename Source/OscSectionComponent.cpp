#include "OscSectionComponent.h"
#include "LookAndFeel.h"

OscSectionComponent::OscSectionComponent (const juce::String& title,
                                           const OscParamIds& ids,
                                           juce::AudioProcessorValueTreeState& avts,
                                           bool embedded)
    : LabelledSectionComponent (title),
      paramIds (ids),
      apvts (avts),
      embedded_ (embedded),
      levelAttach        (avts, ids.level,        levelSlider),
      unisonVoicesAttach (avts, ids.unisonVoices, unisonVoicesSlider),
      unisonDetuneAttach (avts, ids.unisonDetune, unisonDetuneSlider),
      unisonBlendAttach  (avts, ids.unisonBlend,  unisonBlendSlider),
      morphAttach        (avts, ids.morph,         morphSlider),
      coarseTuneAttach   (avts, ids.coarseTune,   coarseTuneSlider),
      fineTuneAttach     (avts, ids.fineTune,     fineTuneSlider)
{
    if (embedded_)
        setSuppressBorder (true);

    BassLookAndFeel::setupRotarySlider (levelSlider, levelLabel, "LEVEL", *this);

    BassLookAndFeel::setupRotarySlider (unisonVoicesSlider, unisonVoicesLabel, "VOICES", *this);
    unisonVoicesSlider.setNumDecimalPlacesToDisplay (0);
    BassLookAndFeel::setupRotarySlider (unisonDetuneSlider, unisonDetuneLabel, "DETUNE", *this);
    BassLookAndFeel::setupRotarySlider (unisonBlendSlider,  unisonBlendLabel,  "BLEND",  *this);

    // Morph knob — rotary
    BassLookAndFeel::setupRotarySlider (morphSlider, morphLabel, "MORPH", *this);

    BassLookAndFeel::setupRotarySlider (coarseTuneSlider, coarseTuneLabel, "COARSE", *this);
    coarseTuneSlider.setNumDecimalPlacesToDisplay (0);
    BassLookAndFeel::setupRotarySlider (fineTuneSlider, fineTuneLabel, "FINE", *this);

    configureOctaveButtons();

    updateButtonStates();
}

void OscSectionComponent::configureOctaveButtons()
{
    octLabel.setText ("OCTAVE", juce::dontSendNotification);
    octLabel.setFont (juce::Font (UIConst::uiFontSize));
    octLabel.setColour (juce::Label::textColourId, BassLookAndFeel::textDim);
    octLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (octLabel);

    const int octGroup = juce::Random::getSystemRandom().nextInt (0x7fffffff);
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
        auto* p = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (paramIds.octaveShift));
        if (p) *p = idx;
    };

    octBtn0.onClick = [setOctParam] { setOctParam (0); };
    octBtn1.onClick = [setOctParam] { setOctParam (1); };
    octBtn2.onClick = [setOctParam] { setOctParam (2); };
}

OscSectionComponent::~OscSectionComponent()
{
    setLookAndFeel (nullptr);
}

void OscSectionComponent::updateButtonStates()
{
    const int octIdx = (int) apvts.getRawParameterValue (paramIds.octaveShift)->load();
    octBtn0.setToggleState (octIdx == 0, juce::dontSendNotification);
    octBtn1.setToggleState (octIdx == 1, juce::dontSendNotification);
    octBtn2.setToggleState (octIdx == 2, juce::dontSendNotification);

    const bool multiFrame = getNumFrames && (getNumFrames() > 1);
    morphSlider.setEnabled (multiFrame);
    morphLabel .setEnabled (multiFrame);
    morphSlider.setAlpha (multiFrame ? 1.0f : 0.35f);
    morphLabel .setAlpha (multiFrame ? 1.0f : 0.35f);

}

void OscSectionComponent::resized()
{
    auto inner = getLocalBounds().reduced (8);
    if (! embedded_)
        inner.removeFromTop (18); // skip section label row

    // Row 1: Level | Morph+EnvMod | Octave  (3 equal columns, 75px tall)
    const int colW3 = inner.getWidth() / 3;
    auto knobRow = inner.removeFromTop (75);
    auto lblRow  = inner.removeFromTop (18);

    // -- Level (col 0)
    levelSlider.setBounds (knobRow.removeFromLeft (colW3));
    levelLabel .setBounds (lblRow .removeFromLeft (colW3));

    // -- Morph (col 1)
    auto morphCol    = knobRow.removeFromLeft (colW3);
    auto morphLblCol = lblRow .removeFromLeft (colW3);

    morphLabel .setBounds (morphLblCol);
    morphSlider.setBounds (morphCol);

    // -- Octave (col 2 — remainder)
    auto octCol = knobRow;
    lblRow.removeFromLeft (lblRow.getWidth()); // consume remainder of label row

    octLabel.setBounds (octCol.removeFromTop (14));
    const int btnH = octCol.getHeight() / 3;
    octBtn0.setBounds (octCol.removeFromTop (btnH));
    octBtn1.setBounds (octCol.removeFromTop (btnH));
    octBtn2.setBounds (octCol);

    inner.removeFromTop (8); // gap

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

    inner.removeFromTop (8); // gap
    const int knobW2 = inner.getWidth() / 2;
    auto knobRow3 = inner.removeFromTop (75);
    coarseTuneSlider.setBounds (knobRow3.removeFromLeft (knobW2));
    fineTuneSlider  .setBounds (knobRow3);
    auto lblRow3 = inner.removeFromTop (18);
    coarseTuneLabel .setBounds (lblRow3.removeFromLeft (knobW2));
    fineTuneLabel   .setBounds (lblRow3);
}
