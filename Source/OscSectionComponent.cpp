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

    BassLookAndFeel::setupLinearSlider (levelSlider,        levelLabel,        "LEVEL",  *this);
    BassLookAndFeel::setupLinearSlider (morphSlider,        morphLabel,        "MORPH",  *this);
    BassLookAndFeel::setupLinearSlider (unisonVoicesSlider, unisonVoicesLabel, "VOICES", *this);
    unisonVoicesSlider.setNumDecimalPlacesToDisplay (0);
    BassLookAndFeel::setupLinearSlider (unisonDetuneSlider, unisonDetuneLabel, "DETUNE", *this);
    BassLookAndFeel::setupLinearSlider (unisonBlendSlider,  unisonBlendLabel,  "BLEND",  *this);

    BassLookAndFeel::setupLinearSlider (coarseTuneSlider,   coarseTuneLabel,   "COARSE", *this);
    coarseTuneSlider.setNumDecimalPlacesToDisplay (0);
    BassLookAndFeel::setupLinearSlider (fineTuneSlider,     fineTuneLabel,     "FINE",   *this);

    updateButtonStates();
}


OscSectionComponent::~OscSectionComponent()
{
    setLookAndFeel (nullptr);
}

void OscSectionComponent::updateButtonStates()
{
    const bool multiFrame = getNumFrames && (getNumFrames() > 1);
    morphSlider.setEnabled (multiFrame);
    morphLabel .setEnabled (multiFrame);
    morphSlider.setAlpha (multiFrame ? 1.0f : 0.35f);
    morphLabel .setAlpha (multiFrame ? 1.0f : 0.35f);

}

void OscSectionComponent::resized()
{
    auto inner = embedded_ ? getLocalBounds() : getLocalBounds().reduced (UIConst::sectionInnerPad);
    if (! embedded_)
        inner.removeFromTop (UIConst::sectionHeaderH);

    const int rowH   = 20;
    const int gap    = 3;
    const int labelW = 46;

    auto sliderRow = [&] (juce::Label& lbl, juce::Slider& sl)
    {
        auto row = inner.removeFromTop (rowH);
        lbl.setBounds (row.removeFromLeft (labelW));
        sl .setBounds (row);
        inner.removeFromTop (gap);
    };

    sliderRow (levelLabel,      levelSlider);
    sliderRow (morphLabel,      morphSlider);
    sliderRow (coarseTuneLabel, coarseTuneSlider);
    sliderRow (fineTuneLabel,   fineTuneSlider);

    inner.removeFromTop (gap);
    dividerY = inner.getY();
    inner.removeFromTop (20);   // separator row — same height as sliders
    inner.removeFromTop (gap);

    sliderRow (unisonVoicesLabel, unisonVoicesSlider);
    sliderRow (unisonDetuneLabel, unisonDetuneSlider);
    sliderRow (unisonBlendLabel,  unisonBlendSlider);
}
