#include "FilterSectionComponent.h"
#include "LookAndFeel.h"

FilterSectionComponent::FilterSectionComponent (JQGunkAudioProcessor& proc,
                                                juce::AudioProcessorValueTreeState& avts)
    : SectionPanel ("FILTER"),
      processor (proc),
      apvts (avts),
      displayComponent (proc, avts),
      filterFreqAttach   (avts, "filterFreq",   filterFreqSlider),
      resonanceAttach    (avts, "envResonance", resonanceSlider),
      freqTrackingAttach (avts, "freqTracking", freqTrackingSlider)
{
    addAndMakeVisible (displayComponent);

    enableBtn.setClickingTogglesState (true);
    addAndMakeVisible (enableBtn);
    enableAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        avts, ParamIDs::filterEnabled, enableBtn);
    setHeaderPower (&enableBtn);
    setEnabledParameter (avts, ParamIDs::filterEnabled);

    // Item IDs are choice index + 1 (the parameter's choices are LP, HP, BP)
    filterTypeCombo.addItemList ({ "Low Pass", "High Pass", "Band Pass" }, 1);
    // Attach after the items exist, so the initial selection is applied.
    filterTypeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        avts, "filterType", filterTypeCombo);
    addAndMakeVisible (filterTypeCombo);
    setHeaderRight (&filterTypeCombo, 140);

    BassLookAndFeel::setupRotarySlider (filterFreqSlider,   filterFreqLabel,   "FREQ",  *this);
    BassLookAndFeel::setupRotarySlider (resonanceSlider,    resonanceLabel,    "RESO",  *this);
    BassLookAndFeel::setupRotarySlider (freqTrackingSlider, freqTrackingLabel, "TRACK", *this);
    filterFreqSlider  .setTooltip ("Filter frequency, as an offset from the tracked pitch");
    resonanceSlider   .setTooltip ("Resonance");
    freqTrackingSlider.setTooltip ("How much the filter follows the detected pitch");
}

void FilterSectionComponent::layoutBody (juce::Rectangle<int> body)
{
    using K = BassLookAndFeel::KnobSize;
    auto knobs = body.removeFromBottom (BassLookAndFeel::knobCellHeight (K::hero));
    body.removeFromBottom (UIConst::knobGap);
    displayComponent.setBounds (body);

    // FREQ and RESO as hero knobs, TRACK small, on a 5-column grid: [ FREQ | RESO | TRACK ]
    const int colW = knobs.getWidth() / 5;
    knobs.removeFromLeft (colW / 2);
    BassLookAndFeel::placeKnob (filterFreqSlider,   filterFreqLabel,   knobs.removeFromLeft (colW * 3 / 2), K::hero);
    BassLookAndFeel::placeKnob (resonanceSlider,    resonanceLabel,    knobs.removeFromLeft (colW * 3 / 2), K::hero);
    BassLookAndFeel::placeKnob (freqTrackingSlider, freqTrackingLabel,
                                knobs.removeFromLeft (colW * 3 / 2).withTrimmedTop (16), K::small);
}
