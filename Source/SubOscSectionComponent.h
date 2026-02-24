#pragma once

#include <JuceHeader.h>
#include "LabelledSectionComponent.h"
#include "LookAndFeel.h"
#include "UIConstants.h"

class SubOscSectionComponent : public LabelledSectionComponent
{
public:
    SubOscSectionComponent (juce::AudioProcessorValueTreeState& avts)
        : LabelledSectionComponent ("SUB"),
          apvts (avts),
          subLevelAttach (avts, "subLevel", subLevelSlider)
    {
        BassLookAndFeel::setupRotarySlider (subLevelSlider, subLevelLabel, "LEVEL", *this);

        octLabel.setText ("OCTAVE", juce::dontSendNotification);
        octLabel.setFont (juce::Font (UIConst::uiFontSize));
        octLabel.setColour (juce::Label::textColourId, BassLookAndFeel::textDim);
        octLabel.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (octLabel);

        const int grp = 103;
        for (auto* b : { &subOctBtn0, &subOctBtn1, &subOctBtn2, &subOctBtn3 })
        {
            b->setRadioGroupId (grp);
            b->setClickingTogglesState (true);
            b->setColour (juce::TextButton::buttonColourId,   BassLookAndFeel::surface);
            b->setColour (juce::TextButton::buttonOnColourId, BassLookAndFeel::accent);
            b->setColour (juce::TextButton::textColourOffId,  BassLookAndFeel::iconDim);
            b->setColour (juce::TextButton::textColourOnId,   juce::Colours::black);
            addAndMakeVisible (b);
        }

        auto setSubOctParam = [this](int idx)
        {
            auto* p = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter ("subOctave"));
            if (p) *p = idx;
        };
        subOctBtn0.onClick = [setSubOctParam] { setSubOctParam (0); };
        subOctBtn1.onClick = [setSubOctParam] { setSubOctParam (1); };
        subOctBtn2.onClick = [setSubOctParam] { setSubOctParam (2); };
        subOctBtn3.onClick = [setSubOctParam] { setSubOctParam (3); };

        bypassFilterBtn.setClickingTogglesState (true);
        bypassFilterBtn.setColour (juce::TextButton::buttonColourId,   BassLookAndFeel::surface);
        bypassFilterBtn.setColour (juce::TextButton::buttonOnColourId, BassLookAndFeel::accent);
        bypassFilterBtn.setColour (juce::TextButton::textColourOffId,  BassLookAndFeel::iconDim);
        bypassFilterBtn.setColour (juce::TextButton::textColourOnId,   juce::Colours::black);
        bypassFilterBtn.onClick = [this]
        {
            auto* p = dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter ("subBypassFilter"));
            if (p) *p = bypassFilterBtn.getToggleState();
        };
        addAndMakeVisible (bypassFilterBtn);

        updateButtonStates();
    }

    ~SubOscSectionComponent() override
    {
        setLookAndFeel (nullptr);
    }

    void updateButtonStates()
    {
        const int octIdx = (int) apvts.getRawParameterValue ("subOctave")->load();
        subOctBtn0.setToggleState (octIdx == 0, juce::dontSendNotification);
        subOctBtn1.setToggleState (octIdx == 1, juce::dontSendNotification);
        subOctBtn2.setToggleState (octIdx == 2, juce::dontSendNotification);
        subOctBtn3.setToggleState (octIdx == 3, juce::dontSendNotification);

        const bool bypass = apvts.getRawParameterValue ("subBypassFilter")->load() > 0.5f;
        bypassFilterBtn.setToggleState (bypass, juce::dontSendNotification);
    }

    void resized() override
    {
        auto inner = getLocalBounds().reduced (8);
        inner.removeFromTop (18); // skip section label row

        // Row 1: level rotary + label
        subLevelSlider.setBounds (inner.removeFromTop (75));
        subLevelLabel .setBounds (inner.removeFromTop (18));

        inner.removeFromTop (6);

        // Row 2: octave label + 4 horizontal buttons
        octLabel.setBounds (inner.removeFromTop (14));
        auto btnRow = inner.removeFromTop (28);
        const int btnW = btnRow.getWidth() / 4;
        subOctBtn0.setBounds (btnRow.removeFromLeft (btnW));
        subOctBtn1.setBounds (btnRow.removeFromLeft (btnW));
        subOctBtn2.setBounds (btnRow.removeFromLeft (btnW));
        subOctBtn3.setBounds (btnRow);

        inner.removeFromTop (6);

        // Row 3: bypass filter toggle
        bypassFilterBtn.setBounds (inner.removeFromTop (28));
    }

private:
    juce::AudioProcessorValueTreeState& apvts;

    juce::Slider subLevelSlider;
    juce::Label  subLevelLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment subLevelAttach;

    juce::Label      octLabel;
    juce::TextButton subOctBtn0 { "-2" }, subOctBtn1 { "-1" },
                     subOctBtn2 {  "0" }, subOctBtn3 { "+1" };

    juce::TextButton bypassFilterBtn { "BYPASS FILT" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SubOscSectionComponent)
};
