#pragma once

#include <JuceHeader.h>
#include "LabelledSectionComponent.h"
#include "LookAndFeel.h"

class OutputSectionComponent : public LabelledSectionComponent,
                                public juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit OutputSectionComponent (juce::AudioProcessorValueTreeState& avts)
        : LabelledSectionComponent ("OUTPUT"),
          apvts (avts),
          volAttach (avts, "masterVolume", volSlider)
    {
        BassLookAndFeel::setupRotarySlider (volSlider, volLabel, "VOL", *this);

        ampEnvLabel.setText ("AMP ENV", juce::dontSendNotification);
        ampEnvLabel.setFont (juce::Font (UIConst::uiFontSize, juce::Font::bold));
        ampEnvLabel.setColour (juce::Label::textColourId, BassLookAndFeel::textDim);
        ampEnvLabel.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (ampEnvLabel);

        for (auto* btn : { &srcButton, &envButton })
        {
            btn->setClickingTogglesState (false);
            btn->setColour (juce::TextButton::buttonColourId,  BassLookAndFeel::surfaceDark);
            btn->setColour (juce::TextButton::buttonOnColourId, BassLookAndFeel::accent.withAlpha (0.8f));
            btn->setColour (juce::TextButton::textColourOffId, juce::Colours::white.withAlpha (0.6f));
            btn->setColour (juce::TextButton::textColourOnId,  juce::Colours::black);
            addAndMakeVisible (btn);
        }

        srcButton.onClick = [this] { apvts.getParameter ("ampEnvSource")->setValueNotifyingHost (0.0f); };
        envButton.onClick = [this] { apvts.getParameter ("ampEnvSource")->setValueNotifyingHost (1.0f); };

        avts.addParameterListener ("ampEnvSource", this);
        updateButtonStates ((int) avts.getRawParameterValue ("ampEnvSource")->load());
    }

    ~OutputSectionComponent() override
    {
        apvts.removeParameterListener ("ampEnvSource", this);
        setLookAndFeel (nullptr);
    }

    void parameterChanged (const juce::String& /*paramID*/, float /*newValue*/) override
    {
        juce::MessageManager::callAsync ([this]
        {
            updateButtonStates ((int) apvts.getRawParameterValue ("ampEnvSource")->load());
        });
    }

    void resized() override
    {
        auto inner = getLocalBounds().reduced (8);
        inner.removeFromTop (18); // skip section label

        inner.removeFromTop (8);
        volSlider.setBounds (inner.removeFromTop (80));
        volLabel .setBounds (inner.removeFromTop (16));

        inner.removeFromTop (12);
        ampEnvLabel.setBounds (inner.removeFromTop (16));
        inner.removeFromTop (4);
        auto btnRow = inner.removeFromTop (28);
        const int btnW = btnRow.getWidth() / 2;
        srcButton.setBounds (btnRow.removeFromLeft (btnW));
        envButton.setBounds (btnRow);
    }

private:
    void updateButtonStates (int idx)
    {
        auto highlight = BassLookAndFeel::accent.withAlpha (0.8f);
        auto normal    = BassLookAndFeel::surfaceDark;

        srcButton.setColour (juce::TextButton::buttonColourId, idx == 0 ? highlight : normal);
        envButton.setColour (juce::TextButton::buttonColourId, idx == 1 ? highlight : normal);
        srcButton.setColour (juce::TextButton::textColourOffId, idx == 0 ? juce::Colours::black : juce::Colours::white.withAlpha (0.6f));
        envButton.setColour (juce::TextButton::textColourOffId, idx == 1 ? juce::Colours::black : juce::Colours::white.withAlpha (0.6f));
        repaint();
    }

    juce::AudioProcessorValueTreeState& apvts;

    juce::Slider volSlider;
    juce::Label  volLabel { "", "VOL" };
    juce::AudioProcessorValueTreeState::SliderAttachment volAttach;

    juce::Label      ampEnvLabel;
    juce::TextButton srcButton { "SRC" }, envButton { "ENV" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OutputSectionComponent)
};
