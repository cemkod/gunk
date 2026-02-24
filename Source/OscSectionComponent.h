#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "LabelledSectionComponent.h"

class OscSectionComponent : public LabelledSectionComponent
{
public:
    OscSectionComponent (JQGunkAudioProcessor& processor,
                         juce::AudioProcessorValueTreeState& apvts);
    ~OscSectionComponent() override;

    void resized() override;

    void updateButtonStates();

private:
    JQGunkAudioProcessor& processor;
    juce::AudioProcessorValueTreeState& apvts;

    juce::Slider levelSlider;
    juce::Label  levelLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment levelAttach;

    juce::Slider unisonVoicesSlider, unisonDetuneSlider, unisonBlendSlider;
    juce::Label  unisonVoicesLabel,  unisonDetuneLabel,  unisonBlendLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment unisonVoicesAttach,
                                                          unisonDetuneAttach,
                                                          unisonBlendAttach;

    juce::DrawableButton waveBtnTri    { "triangle", juce::DrawableButton::ImageFitted };
    juce::DrawableButton waveBtnSq     { "square",   juce::DrawableButton::ImageFitted };
    juce::DrawableButton waveBtnSaw    { "sawtooth", juce::DrawableButton::ImageFitted };
    juce::DrawableButton waveBtnCustom { "custom",   juce::DrawableButton::ImageFitted };

    juce::TextButton octBtn0 { "0" }, octBtn1 { "+1" }, octBtn2 { "+2" };
    juce::Label      octLabel;

    std::unique_ptr<juce::FileChooser> fileChooser;

    void buildWaveformIcons();
    void configureWaveformButtons();
    void configureOctaveButtons();
    void setWaveformParam (int idx);
    void openWavFileDialog();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OscSectionComponent)
};
