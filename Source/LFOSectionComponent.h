#pragma once

#include <JuceHeader.h>
#include "LabelledSectionComponent.h"
#include "LookAndFeel.h"
#include "LFODisplayComponent.h"
#include "UIConstants.h"

class LFOSectionComponent : public LabelledSectionComponent,
                             public juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit LFOSectionComponent (juce::AudioProcessorValueTreeState& avts)
        : LabelledSectionComponent ("LFO"),
          apvts (avts),
          rateAttach   (avts, "lfoRate",   rateSlider),
          amountAttach (avts, "lfoAmount", amountSlider)
    {
        addAndMakeVisible (displayComponent);

        BassLookAndFeel::setupRotarySlider (rateSlider,   rateLabel,   "RATE",   *this);
        BassLookAndFeel::setupRotarySlider (amountSlider, amountLabel, "AMOUNT", *this);

        for (auto* btn : { &sineBtn, &triBtn, &sqBtn, &sawBtn })
        {
            btn->setClickingTogglesState (false);
            btn->setColour (juce::TextButton::buttonColourId,  BassLookAndFeel::surfaceDark);
            btn->setColour (juce::TextButton::buttonOnColourId, BassLookAndFeel::accent.withAlpha (0.8f));
            btn->setColour (juce::TextButton::textColourOffId, juce::Colours::white.withAlpha (0.6f));
            btn->setColour (juce::TextButton::textColourOnId,  juce::Colours::black);
            addAndMakeVisible (btn);
        }

        // lfoShape is a 4-choice parameter; normalised value = index / (numChoices - 1)
        sineBtn.onClick = [this] { apvts.getParameter ("lfoShape")->setValueNotifyingHost (0.0f / 3.0f); };
        triBtn .onClick = [this] { apvts.getParameter ("lfoShape")->setValueNotifyingHost (1.0f / 3.0f); };
        sqBtn  .onClick = [this] { apvts.getParameter ("lfoShape")->setValueNotifyingHost (2.0f / 3.0f); };
        sawBtn .onClick = [this] { apvts.getParameter ("lfoShape")->setValueNotifyingHost (3.0f / 3.0f); };

        avts.addParameterListener ("lfoShape", this);
        updateButtonStates ((int) avts.getRawParameterValue ("lfoShape")->load());
    }

    ~LFOSectionComponent() override
    {
        apvts.removeParameterListener ("lfoShape", this);
        setLookAndFeel (nullptr);
    }

    void parameterChanged (const juce::String& /*paramID*/, float /*newValue*/) override
    {
        juce::MessageManager::callAsync ([this]
        {
            updateButtonStates ((int) apvts.getRawParameterValue ("lfoShape")->load());
        });
    }

    void pushSample (float v) { displayComponent.pushSample (v); }

    void resized() override
    {
        auto inner = getLocalBounds().reduced (8);
        inner.removeFromTop (18); // skip section label

        displayComponent.setBounds (inner.removeFromTop (70));
        inner.removeFromTop (6);

        const int halfW = inner.getWidth() / 2;
        auto knobRow1 = inner.removeFromTop (55);
        rateSlider  .setBounds (knobRow1.removeFromLeft (halfW));
        amountSlider.setBounds (knobRow1);
        auto lblRow1 = inner.removeFromTop (16);
        rateLabel  .setBounds (lblRow1.removeFromLeft (halfW));
        amountLabel.setBounds (lblRow1);

        inner.removeFromTop (8);
        auto btnRow = inner.removeFromTop (28);
        const int btnW = btnRow.getWidth() / 4;
        sineBtn.setBounds (btnRow.removeFromLeft (btnW));
        triBtn .setBounds (btnRow.removeFromLeft (btnW));
        sqBtn  .setBounds (btnRow.removeFromLeft (btnW));
        sawBtn .setBounds (btnRow);
    }

private:
    void updateButtonStates (int idx)
    {
        auto highlight = BassLookAndFeel::accent.withAlpha (0.8f);
        auto normal    = BassLookAndFeel::surfaceDark;
        auto white60   = juce::Colours::white.withAlpha (0.6f);

        sineBtn.setColour (juce::TextButton::buttonColourId,  idx == 0 ? highlight : normal);
        triBtn .setColour (juce::TextButton::buttonColourId,  idx == 1 ? highlight : normal);
        sqBtn  .setColour (juce::TextButton::buttonColourId,  idx == 2 ? highlight : normal);
        sawBtn .setColour (juce::TextButton::buttonColourId,  idx == 3 ? highlight : normal);

        sineBtn.setColour (juce::TextButton::textColourOffId, idx == 0 ? juce::Colours::black : white60);
        triBtn .setColour (juce::TextButton::textColourOffId, idx == 1 ? juce::Colours::black : white60);
        sqBtn  .setColour (juce::TextButton::textColourOffId, idx == 2 ? juce::Colours::black : white60);
        sawBtn .setColour (juce::TextButton::textColourOffId, idx == 3 ? juce::Colours::black : white60);

        repaint();
    }

    juce::AudioProcessorValueTreeState& apvts;

    LFODisplayComponent displayComponent;

    juce::Slider rateSlider;
    juce::Label  rateLabel { "", "RATE" };
    juce::AudioProcessorValueTreeState::SliderAttachment rateAttach;

    juce::Slider amountSlider;
    juce::Label  amountLabel { "", "AMOUNT" };
    juce::AudioProcessorValueTreeState::SliderAttachment amountAttach;

    juce::TextButton sineBtn { "Sine" }, triBtn { "Tri" },
                     sqBtn   { "Sq"   }, sawBtn { "Saw" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LFOSectionComponent)
};
