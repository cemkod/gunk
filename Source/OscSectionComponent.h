#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class OscSectionComponent : public juce::Component
{
public:
    OscSectionComponent (JQGunkAudioProcessor& processor,
                         juce::AudioProcessorValueTreeState& apvts);
    ~OscSectionComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void updateButtonStates();

private:
    JQGunkAudioProcessor& processor;
    juce::AudioProcessorValueTreeState& apvts;

    juce::Slider mixSlider;
    juce::Label  mixLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment mixAttach;

    juce::Slider subLevelSlider;
    juce::Label  subLevelLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment subLevelAttach;

    juce::Slider unisonVoicesSlider, unisonDetuneSlider, unisonBlendSlider;
    juce::Label  unisonVoicesLabel,  unisonDetuneLabel,  unisonBlendLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment unisonVoicesAttach,
                                                          unisonDetuneAttach,
                                                          unisonBlendAttach;

    juce::DrawableButton waveBtnTri    { "triangle", juce::DrawableButton::ImageFitted };
    juce::DrawableButton waveBtnSq     { "square",   juce::DrawableButton::ImageFitted };
    juce::DrawableButton waveBtnSaw    { "sawtooth", juce::DrawableButton::ImageFitted };
    juce::DrawableButton waveBtnCustom { "custom",   juce::DrawableButton::ImageFitted };

    std::unique_ptr<juce::FileChooser> fileChooser;

    void buildWaveformIcons();
    void configureWaveformButtons();
    void setWaveformParam (int idx);
    void openWavFileDialog();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OscSectionComponent)
};
