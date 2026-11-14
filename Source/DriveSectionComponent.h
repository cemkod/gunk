#pragma once

#include <JuceHeader.h>
#include "LabelledSectionComponent.h"
#include "LookAndFeel.h"
#include "UIConstants.h"
#include "PowerToggleButton.h"
#include "ParameterIDs.h"

class DriveSectionComponent : public LabelledSectionComponent
{
public:
    explicit DriveSectionComponent (juce::AudioProcessorValueTreeState& avts)
        : LabelledSectionComponent ("DRIVE"),
          driveAttach (avts, ParamIDs::driveAmount, driveSlider),
          pwrAttach   (avts, ParamIDs::driveEnabled, pwrBtn)
    {
        pwrBtn.setClickingTogglesState (true);
        addAndMakeVisible (pwrBtn);
        BassLookAndFeel::setupRotarySlider (driveSlider, driveLabel, "DRIVE", *this);

        curveLabel.setText ("CURVE", juce::dontSendNotification);
        curveLabel.setFont (BassLookAndFeel::uiFont (UIConst::uiFontSize, true));
        curveLabel.setColour (juce::Label::textColourId, BassLookAndFeel::textDim);
        curveLabel.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (curveLabel);

        // Item IDs are choice index + 1, as ComboBoxAttachment expects.
        curveCombo.addItemList ({ "Soft", "Hard", "Tube" }, 1);
        addAndMakeVisible (curveCombo);
        // Attach after the items exist, so the initial selection is applied.
        curveAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
            avts, ParamIDs::driveCurve, curveCombo);
    }

    ~DriveSectionComponent() override
    {
        setLookAndFeel (nullptr);
    }

    void resized() override
    {
        {
            const int btnH = UIConst::powerToggleH;
            const int btnW = UIConst::powerToggleW;
            auto header = getLocalBounds().removeFromTop (UIConst::sectionHeaderH);
            pwrBtn.setBounds (header.removeFromRight (btnW + UIConst::sectionInnerPad)
                                    .withSizeKeepingCentre (btnW, btnH));
        }

        auto inner = getLocalBounds().reduced (UIConst::sectionInnerPad);
        inner.removeFromTop (UIConst::sectionHeaderH);

        inner.removeFromTop (UIConst::knobGap);
        BassLookAndFeel::placeKnob (driveSlider, driveLabel, inner.removeFromTop (UIConst::knobCellH));

        inner.removeFromTop (UIConst::sectionInnerPad);
        curveLabel.setBounds (inner.removeFromTop (UIConst::knobLabelH));
        inner.removeFromTop (UIConst::knobGap);
        curveCombo.setBounds (inner.removeFromTop (UIConst::buttonH));
    }

private:
    juce::Slider driveSlider;
    juce::Label  driveLabel { "", "DRIVE" };
    juce::AudioProcessorValueTreeState::SliderAttachment driveAttach;

    juce::Label    curveLabel;
    juce::ComboBox curveCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> curveAttach;

    PowerToggleButton pwrBtn;
    juce::AudioProcessorValueTreeState::ButtonAttachment pwrAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DriveSectionComponent)
};
