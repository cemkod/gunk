#pragma once

#include <JuceHeader.h>
#include "SectionPanel.h"
#include "LookAndFeel.h"
#include "EnvelopeDisplayComponent.h"

class EnvelopeSectionComponent : public SectionPanel
{
public:
    explicit EnvelopeSectionComponent (juce::AudioProcessorValueTreeState& avts)
        : SectionPanel ("ENVELOPE"),
          attackAtt  (avts, "modEnvAttack",  attackSlider),
          decayAtt   (avts, "modEnvDecay",   decaySlider),
          sustainAtt (avts, "modEnvSustain", sustainSlider),
          releaseAtt (avts, "modEnvRelease", releaseSlider)
    {
        BassLookAndFeel::setupRotarySlider (attackSlider,  attackLabel,  "ATK", *this);
        BassLookAndFeel::setupRotarySlider (decaySlider,   decayLabel,   "DCY", *this);
        BassLookAndFeel::setupRotarySlider (sustainSlider, sustainLabel, "SUS", *this);
        BassLookAndFeel::setupRotarySlider (releaseSlider, releaseLabel, "REL", *this);

        addAndMakeVisible (displayComponent);
    }

    void resized() override
    {
        auto inner = getLocalBounds().reduced (UIConst::sectionInnerPad);
        inner.removeFromTop (UIConst::sectionHeaderH);

        displayComponent.setBounds (inner.removeFromTop (UIConst::displayH_small));
        inner.removeFromTop (UIConst::knobGap);

        BassLookAndFeel::placeKnobRow (inner.removeFromTop (UIConst::knobCellH),
                                       { { &attackSlider,  &attackLabel },
                                         { &decaySlider,   &decayLabel },
                                         { &sustainSlider, &sustainLabel },
                                         { &releaseSlider, &releaseLabel } });
    }

    void pushSample (float v, float velocity = -1.0f) { displayComponent.pushSample (v, velocity); }

private:
    EnvelopeDisplayComponent displayComponent;
    juce::Slider attackSlider, decaySlider, sustainSlider, releaseSlider;
    juce::Label  attackLabel  { "", "ATK" }, decayLabel  { "", "DCY" },
                 sustainLabel { "", "SUS" }, releaseLabel { "", "REL" };

    juce::AudioProcessorValueTreeState::SliderAttachment attackAtt, decayAtt, sustainAtt, releaseAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EnvelopeSectionComponent)
};
