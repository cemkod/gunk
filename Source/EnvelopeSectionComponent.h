#pragma once

#include <JuceHeader.h>
#include "LabelledSectionComponent.h"
#include "LookAndFeel.h"
#include "EnvelopeDisplayComponent.h"

class EnvelopeSectionComponent : public LabelledSectionComponent
{
public:
    explicit EnvelopeSectionComponent (juce::AudioProcessorValueTreeState& avts)
        : LabelledSectionComponent ("ENVELOPE"),
          attackAtt (avts, "modEnvAttack", attackSlider),
          decayAtt  (avts, "modEnvDecay",  decaySlider)
    {
        BassLookAndFeel::setupRotarySlider (attackSlider, attackLabel, "ATK", *this);
        BassLookAndFeel::setupRotarySlider (decaySlider,  decayLabel,  "DCY", *this);
        addAndMakeVisible (displayComponent);
    }

    void resized() override
    {
        auto inner = getLocalBounds().reduced (UIConst::sectionInnerPad);
        inner.removeFromTop (UIConst::sectionHeaderH); // skip section label

        displayComponent.setBounds (inner.removeFromTop (UIConst::displayH_small));
        inner.removeFromTop (UIConst::knobGap);

        const int knobW = inner.getWidth() / 2;
        auto knobRow = inner.removeFromTop (UIConst::knobRowH);
        attackSlider.setBounds (knobRow.removeFromLeft (knobW));
        decaySlider .setBounds (knobRow);

        auto lblRow = inner.removeFromTop (UIConst::knobLabelH);
        attackLabel.setBounds (lblRow.removeFromLeft (knobW));
        decayLabel .setBounds (lblRow);
    }

    void pushSample (float v) { displayComponent.pushSample (v); }

private:
    EnvelopeDisplayComponent displayComponent;
    juce::Slider attackSlider, decaySlider;
    juce::Label  attackLabel { "", "ATK" }, decayLabel { "", "DCY" };
    juce::AudioProcessorValueTreeState::SliderAttachment attackAtt, decayAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EnvelopeSectionComponent)
};
