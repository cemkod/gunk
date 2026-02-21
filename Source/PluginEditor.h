#pragma once

#include "PluginProcessor.h"

class BassSynthAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit BassSynthAudioProcessorEditor (BassSynthAudioProcessor&);
    ~BassSynthAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    BassSynthAudioProcessor& processor;

    juce::Slider levelSlider;
    juce::Slider mixSlider;

    juce::Label levelLabel;
    juce::Label mixLabel;

    juce::AudioProcessorValueTreeState::SliderAttachment levelAttach;
    juce::AudioProcessorValueTreeState::SliderAttachment mixAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BassSynthAudioProcessorEditor)
};
