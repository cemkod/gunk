#include "FilterSectionComponent.h"
#include "LookAndFeel.h"

FilterSectionComponent::FilterSectionComponent (JQGunkAudioProcessor& proc,
                                                juce::AudioProcessorValueTreeState& avts)
    : LabelledSectionComponent ("FILTER"),
      processor (proc),
      apvts (avts),
      displayComponent (proc, avts),
      filterTypeAttach   (avts, "filterType",    filterTypeCombo),
      filterFreqAttach   (avts, "filterFreq",    filterFreqSlider),
      resonanceAttach    (avts, "envResonance",   resonanceSlider),
      freqTrackingAttach (avts, "freqTracking",   freqTrackingSlider)
{
    addAndMakeVisible (displayComponent);

    filterTypeCombo.addItem ("LP", 1);
    filterTypeCombo.addItem ("HP", 2);
    filterTypeCombo.addItem ("BP", 3);
    addAndMakeVisible (filterTypeCombo);

    BassLookAndFeel::setupRotarySlider (filterFreqSlider,   filterFreqLabel,   "FREQ",  *this);
    BassLookAndFeel::setupRotarySlider (resonanceSlider,    resonanceLabel,    "RESO",  *this);
    BassLookAndFeel::setupRotarySlider (freqTrackingSlider, freqTrackingLabel, "TRACK", *this);
}

FilterSectionComponent::~FilterSectionComponent()
{
    setLookAndFeel (nullptr);
}

void FilterSectionComponent::resized()
{
    auto inner = getLocalBounds().reduced (UIConst::sectionInnerPad);
    inner.removeFromTop (UIConst::sectionHeaderH); // skip section label row

    // Frequency response display
    displayComponent.setBounds (inner.removeFromTop (UIConst::displayH_filter));
    inner.removeFromTop (UIConst::knobGap);

    // Filter type combo box
    filterTypeCombo.setBounds (inner.removeFromTop (UIConst::buttonH));
    inner.removeFromTop (UIConst::knobGap);

    // FREQ | RESO | TRACK knobs
    const int knobW = inner.getWidth() / 3;
    auto knobRow = inner.removeFromTop (UIConst::knobRowH);
    filterFreqSlider  .setBounds (knobRow.removeFromLeft (knobW));
    resonanceSlider   .setBounds (knobRow.removeFromLeft (knobW));
    freqTrackingSlider.setBounds (knobRow.removeFromLeft (knobRow.getWidth()));

    auto lblRow = inner.removeFromTop (UIConst::knobLabelH);
    filterFreqLabel  .setBounds (lblRow.removeFromLeft (knobW));
    resonanceLabel   .setBounds (lblRow.removeFromLeft (knobW));
    freqTrackingLabel.setBounds (lblRow.removeFromLeft (lblRow.getWidth()));
}
