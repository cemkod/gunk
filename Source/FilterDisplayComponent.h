#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class FilterDisplayComponent : public juce::Component
{
public:
    FilterDisplayComponent (JQGunkAudioProcessor& proc,
                            juce::AudioProcessorValueTreeState& apvts);

    void paint (juce::Graphics&) override;

private:
    JQGunkAudioProcessor& processor;
    juce::AudioProcessorValueTreeState& apvts;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterDisplayComponent)
};
