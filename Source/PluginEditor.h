#pragma once

#include "PluginProcessor.h"
#include "LookAndFeel.h"

class JQGunkAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       public juce::Timer
{
public:
    explicit JQGunkAudioProcessorEditor (JQGunkAudioProcessor&);
    ~JQGunkAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    JQGunkAudioProcessor& processor;

    BassLookAndFeel lookAndFeel;

    juce::Slider levelSlider;
    juce::Slider mixSlider;

    juce::Label levelLabel;
    juce::Label mixLabel;

    juce::AudioProcessorValueTreeState::SliderAttachment levelAttach;
    juce::AudioProcessorValueTreeState::SliderAttachment mixAttach;

    juce::Slider sensitivitySlider, resonanceSlider, decaySlider;
    juce::Label  sensitivityLabel,  resonanceLabel,  decayLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment sensitivityAttach, resonanceAttach, decayAttach;

    juce::TextButton sweepBtnOff  { "Off"  };
    juce::TextButton sweepBtnUp   { "Up"   };
    juce::TextButton sweepBtnDown { "Down" };

    juce::Rectangle<int> oscSectionRect, filterSectionRect;

    juce::DrawableButton waveBtnSine   { "sine",     juce::DrawableButton::ImageFitted };
    juce::DrawableButton waveBtnTri    { "triangle", juce::DrawableButton::ImageFitted };
    juce::DrawableButton waveBtnSq     { "square",   juce::DrawableButton::ImageFitted };
    juce::DrawableButton waveBtnSaw    { "sawtooth", juce::DrawableButton::ImageFitted };
    juce::DrawableButton waveBtnCustom { "custom",   juce::DrawableButton::ImageFitted };

    std::unique_ptr<juce::FileChooser> fileChooser;
    std::unique_ptr<juce::Drawable> logo;

    void timerCallback() override;
    void openWavFileDialog();
    void updateWaveButtonStates();
    void setWaveformParam (int idx);
    void setSweepParam (int idx);
    void updateSweepButtonStates();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JQGunkAudioProcessorEditor)
};
