#include "FilterSectionComponent.h"
#include "LookAndFeel.h"

FilterSectionComponent::FilterSectionComponent (JQGunkAudioProcessor& proc,
                                                juce::AudioProcessorValueTreeState& avts)
    : SectionPanel ("FILTER"),
      processor (proc),
      apvts (avts),
      displayComponent (proc, avts),
      filterFreqAttach   (avts, "filterFreq",    filterFreqSlider),
      resonanceAttach    (avts, "envResonance",   resonanceSlider),
      freqTrackingAttach (avts, "freqTracking",   freqTrackingSlider)
{
    addAndMakeVisible (displayComponent);

    enableBtn.setClickingTogglesState (true);
    addAndMakeVisible (enableBtn);
    enableAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        avts, ParamIDs::filterEnabled, enableBtn);

    filterTypeCombo.addItem ("LP", 1);
    filterTypeCombo.addItem ("HP", 2);
    filterTypeCombo.addItem ("BP", 3);
    // Attach after the items exist, so the initial selection is applied.
    filterTypeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        avts, "filterType", filterTypeCombo);
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
    {
        const int btnW = UIConst::powerToggleW, btnH = UIConst::powerToggleH;
        auto header = getLocalBounds().removeFromTop (UIConst::sectionHeaderH);
        auto btnArea = header.removeFromRight (UIConst::sectionInnerPad + btnW)
                             .withSizeKeepingCentre (btnW, btnH);
        enableBtn.setBounds (btnArea);
    }

    auto inner = getLocalBounds().reduced (UIConst::sectionInnerPad);
    inner.removeFromTop (UIConst::sectionHeaderH); // skip section label row

    // Frequency response display
    displayComponent.setBounds (inner.removeFromTop (UIConst::displayH_filter));
    inner.removeFromTop (UIConst::knobGap);

    // Filter type combo box
    filterTypeCombo.setBounds (inner.removeFromTop (UIConst::buttonH));
    inner.removeFromTop (UIConst::knobGap);

    // FREQ | RESO | TRACK knobs
    BassLookAndFeel::placeKnobRow (inner.removeFromTop (UIConst::knobCellH),
                                   { { &filterFreqSlider,   &filterFreqLabel },
                                     { &resonanceSlider,    &resonanceLabel },
                                     { &freqTrackingSlider, &freqTrackingLabel } });
}
