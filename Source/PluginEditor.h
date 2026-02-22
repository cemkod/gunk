#pragma once

#include "PluginProcessor.h"

class BassSynthAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       public juce::Timer
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

    juce::Slider sensitivitySlider, resonanceSlider, decaySlider;
    juce::Label  sensitivityLabel,  resonanceLabel,  decayLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment sensitivityAttach, resonanceAttach, decayAttach;

    juce::ComboBox sweepBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> sweepAttach;

    juce::Rectangle<int> oscSectionRect, filterSectionRect;

    juce::DrawableButton waveBtnSine   { "sine",     juce::DrawableButton::ImageFitted };
    juce::DrawableButton waveBtnTri    { "triangle", juce::DrawableButton::ImageFitted };
    juce::DrawableButton waveBtnSq     { "square",   juce::DrawableButton::ImageFitted };
    juce::DrawableButton waveBtnSaw    { "sawtooth", juce::DrawableButton::ImageFitted };
    juce::DrawableButton waveBtnCustom { "custom",   juce::DrawableButton::ImageFitted };

    std::unique_ptr<juce::FileChooser> fileChooser;

    void timerCallback() override;
    void openWavFileDialog();
    void updateWaveButtonStates();
    void setWaveformParam (int idx);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BassSynthAudioProcessorEditor)
};
