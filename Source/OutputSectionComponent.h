#pragma once

#include <JuceHeader.h>
#include "LabelledSectionComponent.h"
#include "LookAndFeel.h"

class OutputSectionComponent : public LabelledSectionComponent
{
public:
    explicit OutputSectionComponent (juce::AudioProcessorValueTreeState& avts)
        : LabelledSectionComponent ("OUTPUT"),
          apvts (avts),
          volAttach    (avts, "masterVolume", volSlider),
          ampEnvAttach (avts, "ampEnvSource", ampEnvCombo)
    {
        BassLookAndFeel::setupRotarySlider (volSlider, volLabel, "VOL", *this);

        ampEnvLabel.setText ("AMP ENV", juce::dontSendNotification);
        ampEnvLabel.setFont (juce::Font (UIConst::uiFontSize, juce::Font::bold));
        ampEnvLabel.setColour (juce::Label::textColourId, BassLookAndFeel::textDim);
        ampEnvLabel.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (ampEnvLabel);

        ampEnvCombo.addItem ("SRC", 1);
        ampEnvCombo.addItem ("ENV", 2);
        addAndMakeVisible (ampEnvCombo);
    }

    ~OutputSectionComponent() override
    {
        setLookAndFeel (nullptr);
    }

    void resized() override
    {
        auto inner = getLocalBounds().reduced (UIConst::sectionInnerPad);
        inner.removeFromTop (UIConst::sectionHeaderH); // skip section label

        inner.removeFromTop (UIConst::knobGap);
        volSlider.setBounds (inner.removeFromTop (UIConst::knobRowH));
        volLabel .setBounds (inner.removeFromTop (UIConst::knobLabelH));

        inner.removeFromTop (UIConst::sectionInnerPad);
        ampEnvLabel.setBounds (inner.removeFromTop (UIConst::knobLabelH));
        inner.removeFromTop (UIConst::knobGap);
        ampEnvCombo.setBounds (inner.removeFromTop (UIConst::buttonH));
    }

private:
    juce::AudioProcessorValueTreeState& apvts;

    juce::Slider volSlider;
    juce::Label  volLabel { "", "VOL" };
    juce::AudioProcessorValueTreeState::SliderAttachment volAttach;

    juce::Label    ampEnvLabel;
    juce::ComboBox ampEnvCombo;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment ampEnvAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OutputSectionComponent)
};
