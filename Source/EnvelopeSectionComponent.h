#pragma once

#include <JuceHeader.h>
#include "SectionPanel.h"
#include "LookAndFeel.h"
#include "EnvelopeDisplayComponent.h"
#include "SegmentedControl.h"

// ADSR: the mod envelope (modEnv*). Its header also picks what drives the synth's amp:
// the input follower (Input) or this envelope (ADSR).
class EnvelopeSectionComponent : public SectionPanel
{
public:
    explicit EnvelopeSectionComponent (juce::AudioProcessorValueTreeState& avts)
        : SectionPanel ("ADSR"),
          attackAtt  (avts, "modEnvAttack",  attackSlider),
          decayAtt   (avts, "modEnvDecay",   decaySlider),
          sustainAtt (avts, "modEnvSustain", sustainSlider),
          releaseAtt (avts, "modEnvRelease", releaseSlider)
    {
        BassLookAndFeel::setupRotarySlider (attackSlider,  attackLabel,  "ATTACK",  *this);
        BassLookAndFeel::setupRotarySlider (decaySlider,   decayLabel,   "DECAY",   *this);
        BassLookAndFeel::setupRotarySlider (sustainSlider, sustainLabel, "SUSTAIN", *this);
        BassLookAndFeel::setupRotarySlider (releaseSlider, releaseLabel, "RELEASE", *this);

        addAndMakeVisible (displayComponent);

        // ampEnvSource: 0 = the input follower ("Source"), 1 = this envelope ("Env Module")
        ampSource.control.attachToParameter (avts, "ampEnvSource");
        ampSource.setTooltip ("What shapes the synth's volume: the input's own envelope, or this ADSR");
        addAndMakeVisible (ampSource);
        setHeaderRight (&ampSource, 170);
    }

    void pushSample (float v, float velocity = -1.0f) { displayComponent.pushSample (v, velocity); }

private:
    void layoutBody (juce::Rectangle<int> body) override
    {
        const int knobH = BassLookAndFeel::knobCellHeight (BassLookAndFeel::KnobSize::standard);
        BassLookAndFeel::placeKnobRow (body.removeFromBottom (knobH),
                                       { { &attackSlider,  &attackLabel },
                                         { &decaySlider,   &decayLabel },
                                         { &sustainSlider, &sustainLabel },
                                         { &releaseSlider, &releaseLabel } });
        body.removeFromBottom (UIConst::knobGap);
        displayComponent.setBounds (body);
    }

    EnvelopeDisplayComponent displayComponent;
    juce::Slider attackSlider, decaySlider, sustainSlider, releaseSlider;
    juce::Label  attackLabel, decayLabel, sustainLabel, releaseLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment attackAtt, decayAtt, sustainAtt, releaseAtt;

    struct AmpSource : public CaptionedSegmentedControl, public juce::SettableTooltipClient
    {
        AmpSource() : CaptionedSegmentedControl ("AMP", { "Input", "ADSR" }, 34) {}
    } ampSource;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EnvelopeSectionComponent)
};
