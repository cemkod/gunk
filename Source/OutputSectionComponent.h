#pragma once

#include <JuceHeader.h>
#include "LabelledSectionComponent.h"
#include "LookAndFeel.h"
#include "UIConstants.h"
#include "ParameterIDs.h"

class OutputSectionComponent : public LabelledSectionComponent
{
public:
    explicit OutputSectionComponent (juce::AudioProcessorValueTreeState& avts)
        : LabelledSectionComponent ("OUTPUT"),
          apvts (avts),
          volAttach    (avts, "masterVolume", volSlider),
          monoAttach   (avts, ParamIDs::stereoMonoBelow, monoSlider)
    {
        BassLookAndFeel::setupRotarySlider (volSlider, volLabel, "VOL", *this);
        BassLookAndFeel::setupRotarySlider (monoSlider, monoLabel, "MONO <", *this);
        monoSlider.setTooltip ("Unison WIDTH is kept mono below this frequency");

        ampEnvLabel.setText ("AMP ENV", juce::dontSendNotification);
        ampEnvLabel.setFont (BassLookAndFeel::uiFont (UIConst::uiFontSize, true));
        ampEnvLabel.setColour (juce::Label::textColourId, BassLookAndFeel::textDim);
        ampEnvLabel.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (ampEnvLabel);

        ampEnvCombo.addItem ("SRC", 1);
        ampEnvCombo.addItem ("ENV", 2);
        addAndMakeVisible (ampEnvCombo);

        dryPathLabel.setText ("DRY PATH", juce::dontSendNotification);
        dryPathLabel.setFont (BassLookAndFeel::uiFont (UIConst::uiFontSize, true));
        dryPathLabel.setColour (juce::Label::textColourId, BassLookAndFeel::textDim);
        dryPathLabel.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (dryPathLabel);

        dryPathCombo.addItem ("CLEAN", 1);
        dryPathCombo.addItem ("FX", 2);
        dryPathCombo.setTooltip ("FX: the dry input goes through Drive and the Filter with the synth");
        addAndMakeVisible (dryPathCombo);

        // Attach after the items exist, so the initial selection is applied.
        using ComboAttach = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
        ampEnvAttach  = std::make_unique<ComboAttach> (avts, "ampEnvSource",       ampEnvCombo);
        dryPathAttach = std::make_unique<ComboAttach> (avts, ParamIDs::dryRouting, dryPathCombo);
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
        BassLookAndFeel::placeKnobRow (inner.removeFromTop (UIConst::knobCellH),
                                       { { &volSlider,  &volLabel },
                                         { &monoSlider, &monoLabel } });

        inner.removeFromTop (UIConst::sectionInnerPad);
        ampEnvLabel.setBounds (inner.removeFromTop (UIConst::knobLabelH));
        inner.removeFromTop (UIConst::knobGap);
        ampEnvCombo.setBounds (inner.removeFromTop (UIConst::buttonH));

        inner.removeFromTop (UIConst::sectionInnerPad);
        dryPathLabel.setBounds (inner.removeFromTop (UIConst::knobLabelH));
        inner.removeFromTop (UIConst::knobGap);
        dryPathCombo.setBounds (inner.removeFromTop (UIConst::buttonH));
    }

private:
    juce::AudioProcessorValueTreeState& apvts;

    juce::Slider volSlider;
    juce::Label  volLabel { "", "VOL" };
    juce::AudioProcessorValueTreeState::SliderAttachment volAttach;

    juce::Slider monoSlider;
    juce::Label  monoLabel { "", "MONO <" };
    juce::AudioProcessorValueTreeState::SliderAttachment monoAttach;

    juce::Label    ampEnvLabel;
    juce::ComboBox ampEnvCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> ampEnvAttach;

    juce::Label    dryPathLabel;
    juce::ComboBox dryPathCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> dryPathAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OutputSectionComponent)
};
