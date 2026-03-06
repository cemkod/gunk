#pragma once

#include <JuceHeader.h>
#include "LabelledSectionComponent.h"
#include "ParameterIDs.h"
#include "LookAndFeel.h"
#include "UIConstants.h"

class SubOscSectionComponent : public LabelledSectionComponent
{
public:
    SubOscSectionComponent (juce::AudioProcessorValueTreeState& avts)
        : LabelledSectionComponent ("SUB"),
          apvts (avts),
          subLevelAttach (avts, ParamIDs::subLevel,  subLevelSlider),
          subOctAttach   (avts, ParamIDs::subOctave, subOctCombo)
    {
        BassLookAndFeel::setupRotarySlider (subLevelSlider, subLevelLabel, "LEVEL", *this);

        octLabel.setText ("OCTAVE", juce::dontSendNotification);
        octLabel.setFont (juce::Font (UIConst::uiFontSize));
        octLabel.setColour (juce::Label::textColourId, BassLookAndFeel::textDim);
        octLabel.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (octLabel);

        subOctCombo.addItem ("-2", 1);
        subOctCombo.addItem ("-1", 2);
        subOctCombo.addItem ( "0", 3);
        subOctCombo.addItem ("+1", 4);
        addAndMakeVisible (subOctCombo);

        bypassFilterBtn.setClickingTogglesState (true);
        bypassFilterBtn.setColour (juce::TextButton::buttonColourId,   BassLookAndFeel::surface);
        bypassFilterBtn.setColour (juce::TextButton::buttonOnColourId, BassLookAndFeel::accent);
        bypassFilterBtn.setColour (juce::TextButton::textColourOffId,  BassLookAndFeel::iconDim);
        bypassFilterBtn.setColour (juce::TextButton::textColourOnId,   juce::Colours::black);
        bypassFilterBtn.onClick = [this]
        {
            auto* p = dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter (ParamIDs::subBypassFilter));
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
        const bool bypass = apvts.getRawParameterValue (ParamIDs::subBypassFilter)->load() > 0.5f;
        bypassFilterBtn.setToggleState (bypass, juce::dontSendNotification);
    }

    void resized() override
    {
        auto inner = getLocalBounds().reduced (UIConst::sectionInnerPad);
        inner.removeFromTop (UIConst::sectionHeaderH); // skip section label row

        // Row 1: level rotary + label
        subLevelSlider.setBounds (inner.removeFromTop (UIConst::knobRowH));
        subLevelLabel .setBounds (inner.removeFromTop (UIConst::knobLabelH));

        inner.removeFromTop (UIConst::knobGap);

        // Row 2: octave label + combo
        octLabel  .setBounds (inner.removeFromTop (UIConst::knobLabelH));
        subOctCombo.setBounds (inner.removeFromTop (UIConst::buttonH));

        inner.removeFromTop (UIConst::knobGap);

        // Row 3: bypass filter toggle
        bypassFilterBtn.setBounds (inner.removeFromTop (UIConst::buttonH));
    }

private:
    juce::AudioProcessorValueTreeState& apvts;

    juce::Slider subLevelSlider;
    juce::Label  subLevelLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment subLevelAttach;

    juce::Label    octLabel;
    juce::ComboBox subOctCombo;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment subOctAttach;

    juce::TextButton bypassFilterBtn { "BYPASS FILT" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SubOscSectionComponent)
};
