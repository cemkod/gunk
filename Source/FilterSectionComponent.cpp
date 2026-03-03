#include "FilterSectionComponent.h"
#include "LookAndFeel.h"

FilterSectionComponent::FilterSectionComponent (JQGunkAudioProcessor& proc,
                                                juce::AudioProcessorValueTreeState& avts)
    : LabelledSectionComponent ("Filter"),
      processor (proc),
      apvts (avts),
      displayComponent (proc, avts),
      filterFreqAttach   (avts, "filterFreq",    filterFreqSlider),
      resonanceAttach    (avts, "envResonance",   resonanceSlider),
      freqTrackingAttach (avts, "freqTracking",   freqTrackingSlider)
{
    addAndMakeVisible (displayComponent);

    BassLookAndFeel::setupRotarySlider (filterFreqSlider,   filterFreqLabel,   "FREQ",  *this);
    BassLookAndFeel::setupRotarySlider (resonanceSlider,    resonanceLabel,    "RESO",  *this);
    BassLookAndFeel::setupRotarySlider (freqTrackingSlider, freqTrackingLabel, "TRACK", *this);

    // LP / HP / BP mode buttons
    for (auto* btn : { &lpButton, &hpButton, &bpButton })
    {
        btn->setClickingTogglesState (false);
        btn->setColour (juce::TextButton::buttonColourId,     BassLookAndFeel::surfaceDark);
        btn->setColour (juce::TextButton::buttonOnColourId,   BassLookAndFeel::accent.withAlpha (0.8f));
        btn->setColour (juce::TextButton::textColourOffId,    juce::Colours::white.withAlpha (0.6f));
        btn->setColour (juce::TextButton::textColourOnId,     juce::Colours::black);
        addAndMakeVisible (btn);
    }

    lpButton.onClick = [this] { apvts.getParameter ("filterType")->setValueNotifyingHost (0.0f); };
    hpButton.onClick = [this] { apvts.getParameter ("filterType")->setValueNotifyingHost (1.0f / 2.0f); };
    bpButton.onClick = [this] { apvts.getParameter ("filterType")->setValueNotifyingHost (1.0f); };

    avts.addParameterListener ("filterType", this);
    updateButtonStates ((int) avts.getRawParameterValue ("filterType")->load());
}

FilterSectionComponent::~FilterSectionComponent()
{
    apvts.removeParameterListener ("filterType", this);
    setLookAndFeel (nullptr);
}

void FilterSectionComponent::parameterChanged (const juce::String& /*paramID*/, float /*newValue*/)
{
    juce::MessageManager::callAsync ([this]
    {
        updateButtonStates ((int) apvts.getRawParameterValue ("filterType")->load());
    });
}

void FilterSectionComponent::updateButtonStates (int filterTypeIdx)
{
    lpButton.setToggleState (filterTypeIdx == 0, juce::dontSendNotification);
    hpButton.setToggleState (filterTypeIdx == 1, juce::dontSendNotification);
    bpButton.setToggleState (filterTypeIdx == 2, juce::dontSendNotification);

    auto highlight = BassLookAndFeel::accent.withAlpha (0.8f);
    auto normal    = BassLookAndFeel::surfaceDark;

    lpButton.setColour (juce::TextButton::buttonColourId, filterTypeIdx == 0 ? highlight : normal);
    hpButton.setColour (juce::TextButton::buttonColourId, filterTypeIdx == 1 ? highlight : normal);
    bpButton.setColour (juce::TextButton::buttonColourId, filterTypeIdx == 2 ? highlight : normal);

    lpButton.setColour (juce::TextButton::textColourOffId, filterTypeIdx == 0 ? juce::Colours::black : juce::Colours::white.withAlpha (0.6f));
    hpButton.setColour (juce::TextButton::textColourOffId, filterTypeIdx == 1 ? juce::Colours::black : juce::Colours::white.withAlpha (0.6f));
    bpButton.setColour (juce::TextButton::textColourOffId, filterTypeIdx == 2 ? juce::Colours::black : juce::Colours::white.withAlpha (0.6f));

    repaint();
    displayComponent.repaint();
}

void FilterSectionComponent::resized()
{
    auto inner = getLocalBounds().reduced (8);
    inner.removeFromTop (18); // skip section label row

    // Frequency response display
    displayComponent.setBounds (inner.removeFromTop (90));
    inner.removeFromTop (8);

    // LP / HP / BP button row
    auto btnRow = inner.removeFromTop (22);
    const int btnW = btnRow.getWidth() / 3;
    lpButton.setBounds (btnRow.removeFromLeft (btnW));
    hpButton.setBounds (btnRow.removeFromLeft (btnW));
    bpButton.setBounds (btnRow.removeFromLeft (btnRow.getWidth()));
    inner.removeFromTop (8);

    // FREQ | RESO | TRACK knobs
    const int knobW = inner.getWidth() / 3;
    auto knobRow = inner.removeFromTop (75);
    filterFreqSlider  .setBounds (knobRow.removeFromLeft (knobW));
    resonanceSlider   .setBounds (knobRow.removeFromLeft (knobW));
    freqTrackingSlider.setBounds (knobRow.removeFromLeft (knobRow.getWidth()));

    auto lblRow = inner.removeFromTop (18);
    filterFreqLabel  .setBounds (lblRow.removeFromLeft (knobW));
    resonanceLabel   .setBounds (lblRow.removeFromLeft (knobW));
    freqTrackingLabel.setBounds (lblRow.removeFromLeft (lblRow.getWidth()));
}
