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
          rateAttach   (avts, "lfoRate",   rateSlider),
          amountAttach (avts, "lfoAmount", amountSlider)
    {
        addAndMakeVisible (displayComponent);

        BassLookAndFeel::setupRotarySlider (rateSlider,   rateLabel,   "RATE",   *this);
        BassLookAndFeel::setupRotarySlider (amountSlider, amountLabel, "AMOUNT", *this);

        // Item IDs are choice index + 1
        shapeCombo.addItemList ({ "Sine", "Triangle", "Square", "Saw" }, 1);
        // Attach after the items exist, so the initial selection is applied.
        shapeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (avts, "lfoShape", shapeCombo);
        addAndMakeVisible (shapeCombo);
        BassLookAndFeel::setupCaption (shapeLabel, "SHAPE", *this);
    }

    void pushSample (float v) { displayComponent.pushSample (v); }

private:
    void layoutBody (juce::Rectangle<int> body) override
    {
        // [ display | RATE | AMOUNT | SHAPE ]
        displayComponent.setBounds (body.removeFromLeft (body.getWidth() * 36 / 100));
        body.removeFromLeft (UIConst::knobGap);

        auto shape = body.removeFromRight (body.getWidth() * 36 / 100);
        shape = shape.withSizeKeepingCentre (shape.getWidth(), UIConst::captionH + 4 + UIConst::buttonH);
        shapeLabel.setBounds (shape.removeFromTop (UIConst::captionH));
        shape.removeFromTop (4);
        shapeCombo.setBounds (shape);
        body.removeFromRight (UIConst::knobGap);

        BassLookAndFeel::placeKnobRow (body, { { &rateSlider,   &rateLabel },
                                               { &amountSlider, &amountLabel } });
    }

    LFODisplayComponent displayComponent;

    juce::ComboBox shapeCombo;
    juce::Label    shapeLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shapeAttach;

    juce::Slider rateSlider;
    juce::Label  rateLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment rateAttach;

    juce::Slider amountSlider;
    juce::Label  amountLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment amountAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LFOSectionComponent)
};
