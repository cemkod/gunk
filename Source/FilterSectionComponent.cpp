#include "FilterSectionComponent.h"
#include "LookAndFeel.h"

FilterSectionComponent::FilterSectionComponent (JQGunkAudioProcessor& proc,
                                                juce::AudioProcessorValueTreeState& avts)
    : LabelledSectionComponent ("ENV FILTER"),
      processor (proc),
      apvts (avts),
      displayComponent (proc, avts),
      filterFreqAttach   (avts, "filterFreq",  filterFreqSlider),
      resonanceAttach    (avts, "envResonance", resonanceSlider),
      decayAttach        (avts, "envDecay",     decaySlider),
      freqTrackingAttach (avts, "freqTracking", freqTrackingSlider)
{
    addAndMakeVisible (displayComponent);

    BassLookAndFeel::setupRotarySlider (filterFreqSlider,   filterFreqLabel,   "FREQ",  *this);
    BassLookAndFeel::setupRotarySlider (resonanceSlider,    resonanceLabel,    "RESO",  *this);
    BassLookAndFeel::setupRotarySlider (decaySlider,        decayLabel,        "DECAY", *this);
    BassLookAndFeel::setupRotarySlider (freqTrackingSlider, freqTrackingLabel, "TRACK", *this);
}

FilterSectionComponent::~FilterSectionComponent()
{
    setLookAndFeel (nullptr);
}

void FilterSectionComponent::resized()
{
    auto inner = getLocalBounds().reduced (8);
    inner.removeFromTop (18); // skip section label row

    // Frequency response display
    displayComponent.setBounds (inner.removeFromTop (90));
    inner.removeFromTop (18);

    // Freq | Resonance | Decay | Track knobs
    const int knobW = inner.getWidth() / 4;
    auto knobRow = inner.removeFromTop (75);
    filterFreqSlider  .setBounds (knobRow.removeFromLeft (knobW));
    resonanceSlider   .setBounds (knobRow.removeFromLeft (knobW));
    decaySlider       .setBounds (knobRow.removeFromLeft (knobW));
    freqTrackingSlider.setBounds (knobRow.removeFromLeft (knobW));

    auto lblRow = inner.removeFromTop (18);
    filterFreqLabel  .setBounds (lblRow.removeFromLeft (knobW));
    resonanceLabel   .setBounds (lblRow.removeFromLeft (knobW));
    decayLabel       .setBounds (lblRow.removeFromLeft (knobW));
    freqTrackingLabel.setBounds (lblRow.removeFromLeft (knobW));
}
