#include "OscSectionComponent.h"

OscSectionComponent::OscSectionComponent (const OscParamIds& ids,
                                          juce::AudioProcessorValueTreeState& avts)
    : levelAttach        (avts, ids.level,        levelSlider),
      morphAttach        (avts, ids.morph,        morphSlider),
      coarseTuneAttach   (avts, ids.coarseTune,   coarseTuneSlider),
      fineTuneAttach     (avts, ids.fineTune,     fineTuneSlider),
      unisonVoicesAttach (avts, ids.unisonVoices, unisonVoicesSlider),
      unisonDetuneAttach (avts, ids.unisonDetune, unisonDetuneSlider),
      unisonBlendAttach  (avts, ids.unisonBlend,  unisonBlendSlider),
      unisonWidthAttach  (avts, ids.unisonWidth,  unisonWidthSlider)
{
    // Only the knobs take the mouse; the panel's displays and combos sit underneath.
    setInterceptsMouseClicks (false, true);

    BassLookAndFeel::setupRotarySlider (levelSlider,        levelLabel,        "LEVEL",  *this);
    BassLookAndFeel::setupRotarySlider (morphSlider,        morphLabel,        "MORPH",  *this);
    BassLookAndFeel::setupRotarySlider (coarseTuneSlider,   coarseTuneLabel,   "COARSE", *this);
    BassLookAndFeel::setupRotarySlider (fineTuneSlider,     fineTuneLabel,     "FINE",   *this);
    BassLookAndFeel::setupRotarySlider (unisonVoicesSlider, unisonVoicesLabel, "VOICES", *this);
    BassLookAndFeel::setupRotarySlider (unisonDetuneSlider, unisonDetuneLabel, "DETUNE", *this);
    BassLookAndFeel::setupRotarySlider (unisonBlendSlider,  unisonBlendLabel,  "BLEND",  *this);
    BassLookAndFeel::setupRotarySlider (unisonWidthSlider,  unisonWidthLabel,  "WIDTH",  *this);
    coarseTuneSlider  .setNumDecimalPlacesToDisplay (0);
    unisonVoicesSlider.setNumDecimalPlacesToDisplay (0);

    morphSlider       .setTooltip ("Position in the wavetable (needs a multi-frame wavetable)");
    unisonBlendSlider .setTooltip ("Level of the outer unison voices relative to the centre");
    unisonWidthSlider .setTooltip ("Stereo spread of the unison voices");

    updateButtonStates();
}

void OscSectionComponent::updateButtonStates()
{
    // MORPH only means something with a multi-frame wavetable
    const bool multiFrame = getNumFrames && (getNumFrames() > 1);
    morphSlider.setEnabled (multiFrame);
    morphSlider.setAlpha (multiFrame ? 1.0f : 0.4f);
    morphLabel .setAlpha (multiFrame ? 1.0f : 0.4f);
}

void OscSectionComponent::layoutKnobs (juce::Rectangle<int> mainRow, juce::Rectangle<int> unisonRow)
{
    BassLookAndFeel::placeKnobRow (mainRow, { { &levelSlider,      &levelLabel },
                                              { &morphSlider,      &morphLabel },
                                              { &coarseTuneSlider, &coarseTuneLabel },
                                              { &fineTuneSlider,   &fineTuneLabel } });
    BassLookAndFeel::placeKnobRow (unisonRow, { { &unisonVoicesSlider, &unisonVoicesLabel },
                                                { &unisonDetuneSlider, &unisonDetuneLabel },
                                                { &unisonBlendSlider,  &unisonBlendLabel },
                                                { &unisonWidthSlider,  &unisonWidthLabel } });
}
