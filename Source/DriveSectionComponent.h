#pragma once

#include <JuceHeader.h>
#include "SectionPanel.h"
#include "LookAndFeel.h"
#include "UIConstants.h"
#include "PowerToggleButton.h"
#include "SegmentedControl.h"
#include "ParameterIDs.h"
#include "ModMatrix.h"

class DriveSectionComponent : public SectionPanel
{
public:
    explicit DriveSectionComponent (juce::AudioProcessorValueTreeState& avts)
        : SectionPanel ("DRIVE"),
          driveAttach (avts, ParamIDs::driveAmount, driveSlider),
          pwrAttach   (avts, ParamIDs::driveEnabled, pwrBtn)
    {
        pwrBtn.setClickingTogglesState (true);
        addAndMakeVisible (pwrBtn);
        setHeaderPower (&pwrBtn);
        setEnabledParameter (avts, ParamIDs::driveEnabled);

        BassLookAndFeel::setupRotarySlider (driveSlider, driveLabel, "DRIVE", *this);
        BassLookAndFeel::setModTarget (driveSlider, (int) ModTarget::Drive);

        // Segment index = choice index (Soft, Hard, Tube)
        curveSwitch.attachToParameter (avts, ParamIDs::driveCurve);
        addAndMakeVisible (curveSwitch);
    }

private:
    void layoutBody (juce::Rectangle<int> body) override
    {
        curveSwitch.setBounds (body.removeFromBottom (32));
        body.removeFromBottom (UIConst::knobGap);
        BassLookAndFeel::placeKnob (driveSlider, driveLabel, body, BassLookAndFeel::KnobSize::hero);
    }

    juce::Slider driveSlider;
    juce::Label  driveLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment driveAttach;

    SegmentedControl curveSwitch { { "Soft", "Hard", "Tube" } };

    PowerToggleButton pwrBtn;
    juce::AudioProcessorValueTreeState::ButtonAttachment pwrAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DriveSectionComponent)
};
