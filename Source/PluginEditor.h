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

    juce::ComboBox waveformBox;
    juce::Label    waveformLabel;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment waveformAttach;
    juce::TextButton loadWavButton { "Load WAV" };
    std::unique_ptr<juce::FileChooser> fileChooser; // must outlive async callback

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BassSynthAudioProcessorEditor)
};
