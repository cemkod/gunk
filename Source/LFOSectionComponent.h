#pragma once

#include <JuceHeader.h>
#include "SectionPanel.h"
#include "LookAndFeel.h"
#include "LFODisplayComponent.h"
#include "UIConstants.h"

class LFOSectionComponent : public SectionPanel
{
public:
    explicit LFOSectionComponent (juce::AudioProcessorValueTreeState& avts)
        : SectionPanel ("LFO"),
          apvts (avts),
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
        // Attach after the items exist, so the initial selection is applied.
        shapeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (avts, "lfoShape", shapeCombo);
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

        BassLookAndFeel::placeKnobRow (inner.removeFromTop (UIConst::knobCellH),
                                       { { &rateSlider,   &rateLabel },
                                         { &amountSlider, &amountLabel } });

        inner.removeFromTop (UIConst::knobGap);
        shapeCombo.setBounds (inner.removeFromTop (UIConst::buttonH));
    }

private:
    juce::AudioProcessorValueTreeState& apvts;

    LFODisplayComponent displayComponent;

    juce::ComboBox shapeCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shapeAttach;

    juce::Slider rateSlider;
    juce::Label  rateLabel { "", "RATE" };
    juce::AudioProcessorValueTreeState::SliderAttachment rateAttach;

    juce::Slider amountSlider;
    juce::Label  amountLabel { "", "AMOUNT" };
    juce::AudioProcessorValueTreeState::SliderAttachment amountAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LFOSectionComponent)
};
