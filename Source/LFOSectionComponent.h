#pragma once

#include <JuceHeader.h>
#include "LabelledSectionComponent.h"
#include "LookAndFeel.h"
#include "LFODisplayComponent.h"
#include "UIConstants.h"

class LFOSectionComponent : public LabelledSectionComponent
{
public:
    explicit LFOSectionComponent (juce::AudioProcessorValueTreeState& avts)
        : LabelledSectionComponent ("LFO"),
          apvts (avts),
          shapeAttach  (avts, "lfoShape",  shapeCombo),
          rateAttach   (avts, "lfoRate",   rateSlider),
          amountAttach (avts, "lfoAmount", amountSlider)
    {
        addAndMakeVisible (displayComponent);

        BassLookAndFeel::setupRotarySlider (rateSlider,   rateLabel,   "RATE",   *this);
        BassLookAndFeel::setupRotarySlider (amountSlider, amountLabel, "AMOUNT", *this);

        shapeCombo.addItem ("Sine", 1);
        shapeCombo.addItem ("Tri",  2);
        shapeCombo.addItem ("Sq",   3);
        shapeCombo.addItem ("Saw",  4);
        addAndMakeVisible (shapeCombo);
    }

    ~LFOSectionComponent() override
    {
        setLookAndFeel (nullptr);
    }

    void pushSample (float v) { displayComponent.pushSample (v); }

    void resized() override
    {
        auto inner = getLocalBounds().reduced (UIConst::sectionInnerPad);
        inner.removeFromTop (UIConst::sectionHeaderH); // skip section label

        displayComponent.setBounds (inner.removeFromTop (UIConst::displayH_small));
        inner.removeFromTop (UIConst::knobGap);

        const int halfW = inner.getWidth() / 2;
        auto knobRow1 = inner.removeFromTop (UIConst::knobRowH);
        rateSlider  .setBounds (knobRow1.removeFromLeft (halfW));
        amountSlider.setBounds (knobRow1);
        auto lblRow1 = inner.removeFromTop (UIConst::knobLabelH);
        rateLabel  .setBounds (lblRow1.removeFromLeft (halfW));
        amountLabel.setBounds (lblRow1);

        inner.removeFromTop (UIConst::knobGap);
        shapeCombo.setBounds (inner.removeFromTop (UIConst::buttonH));
    }

private:
    juce::AudioProcessorValueTreeState& apvts;

    LFODisplayComponent displayComponent;

    juce::ComboBox shapeCombo;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment shapeAttach;

    juce::Slider rateSlider;
    juce::Label  rateLabel { "", "RATE" };
    juce::AudioProcessorValueTreeState::SliderAttachment rateAttach;

    juce::Slider amountSlider;
    juce::Label  amountLabel { "", "AMOUNT" };
    juce::AudioProcessorValueTreeState::SliderAttachment amountAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LFOSectionComponent)
};
