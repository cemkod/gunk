#pragma once

#include <JuceHeader.h>
#include "FilterDisplayComponent.h"
#include "PluginProcessor.h"
#include "SectionPanel.h"
#include "ParameterIDs.h"
#include "PowerToggleButton.h"

class FilterSectionComponent : public SectionPanel
{
public:
    FilterSectionComponent (JQGunkAudioProcessor& proc,
                            juce::AudioProcessorValueTreeState& apvts);

    void repaintDisplay()
    {
        const float cutoff     = processor.getCurrentCutoffHz();
        const float q          = apvts.getRawParameterValue (ParamIDs::envResonance)->load();
        const int   filterType = (int) apvts.getRawParameterValue (ParamIDs::filterType)->load();

        if (std::abs (cutoff - lastCutoff) > 1.0f || std::abs (q - lastQ) > 0.01f || filterType != lastFilterType)
        {
            lastCutoff     = cutoff;
            lastQ          = q;
            lastFilterType = filterType;
            displayComponent.repaint();
        }
    }

private:
    void layoutBody (juce::Rectangle<int> body) override;

    JQGunkAudioProcessor& processor;
    juce::AudioProcessorValueTreeState& apvts;

    FilterDisplayComponent displayComponent;

    PowerToggleButton enableBtn;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableAttach;

    juce::ComboBox filterTypeCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> filterTypeAttach;

    juce::Slider filterFreqSlider, resonanceSlider, freqTrackingSlider;
    juce::Label  filterFreqLabel,  resonanceLabel,  freqTrackingLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment filterFreqAttach, resonanceAttach, freqTrackingAttach;

    float lastCutoff     = -1.0f;
    float lastQ          = -1.0f;
    int   lastFilterType = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterSectionComponent)
};
