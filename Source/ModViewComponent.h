#pragma once

#include <JuceHeader.h>
#include <array>
#include "PluginProcessor.h"
#include "ModMatrix.h"
#include "SectionPanel.h"
#include "IconButton.h"
#include "EnvelopeDisplayComponent.h"
#include "LFODisplayComponent.h"

// Target picker grouped by section. Item IDs are ModTarget values (choice index + 1).
class TargetComboBox : public juce::ComboBox
{
public:
    void showPopup() override;
};

//==============================================================================
// Scrolling pitch trace (log 40..400 Hz) with the current note in a box at the right.
class PitchTraceComponent : public juce::Component
{
public:
    PitchTraceComponent() : history (160, 0.0f) {}

    void setLineColour (juce::Colour c) { lineColour = c; }
    void pushSample (float hz);
    void paint (juce::Graphics&) override;

private:
    std::vector<float> history;
    int writePos = 0;
    juce::Colour lineColour;
    juce::String note { "--" };
};

//==============================================================================
// One row of the matrix:  [n] [ * Source v ] -> [ Target v ] [ ----|####---- ] +0.25 [x]
// When both source and target are None the row shows only [n] and "+ Add modulation".
class ModSlotRow : public juce::Component
{
public:
    ModSlotRow (juce::AudioProcessorValueTreeState& apvts, int slotIndex);

    // Re-reads the slot's parameters; called from the timer, so preset loads and
    // automation keep the row's look in step with no separate UI state.
    void sync();
    bool isEmptySlot() const { return empty; }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void setEmptyLook (bool isEmpty);
    void showAddMenu();
    void clearSlot();
    static void setParamWithGesture (juce::RangedAudioParameter*, float denormalisedValue);

    const int slot;
    juce::RangedAudioParameter* sourceParam;
    juce::RangedAudioParameter* targetParam;
    juce::RangedAudioParameter* amountParam;

    juce::ComboBox sourceCombo;
    TargetComboBox targetCombo;
    juce::Slider   amountSlider;
    juce::Label    amountValue;
    IconButton     clearBtn { IconButton::Icon::close, true };
    // Dashed "+ Add modulation" placeholder for an empty slot
    struct AddButton : public juce::Button
    {
        AddButton() : juce::Button ("Add modulation") {}
        void paintButton (juce::Graphics&, bool over, bool down) override;
    } addBtn;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> sourceAttach, targetAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   amountAttach;

    bool empty = false;
    int  lastSource = -1;
    juce::Rectangle<int> numberBox, arrowArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModSlotRow)
};

//==============================================================================
// The MOD screen: a column of live source cards and the 8-slot matrix.
class ModViewComponent : public juce::Component
{
public:
    explicit ModViewComponent (JQGunkAudioProcessor& p);

    void resized() override;

    // Called from the editor timer while this view is visible.
    void tick();

private:
    JQGunkAudioProcessor& processor;

    // Source cards. Colours and names come from BassLookAndFeel::modSource*,
    // indexed by the source choice (1 = Input Env, 2 = Pitch, 3 = ADSR, 4 = LFO).
    // A SectionPanel whose body layout is a lambda
    struct Panel : public SectionPanel
    {
        using SectionPanel::SectionPanel;
        std::function<void (juce::Rectangle<int>)> onLayout;
        void layoutBody (juce::Rectangle<int> body) override { if (onLayout) onLayout (body); }
    };
    struct Card : public Panel
    {
        explicit Card (int sourceIndex)
            : Panel (BassLookAndFeel::modSourceNames[sourceIndex].toUpperCase())
        {
            setTitleDot (BassLookAndFeel::modSourceColour (sourceIndex));
        }
    };

    Card inputCard { 1 }, pitchCard { 2 }, adsrCard { 3 }, lfoCard { 4 };

    EnvelopeDisplayComponent inputTrace;
    PitchTraceComponent      pitchTrace;
    EnvelopeDisplayComponent adsrTrace;
    LFODisplayComponent      lfoTrace;

    juce::Slider attackSlider, decaySlider, sustainSlider, releaseSlider, rateSlider, amountSlider;
    juce::Label  attackLabel,  decayLabel,  sustainLabel,  releaseLabel,  rateLabel,  amountLabel, shapeLabel;
    juce::ComboBox shapeCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAtt, decayAtt, sustainAtt, releaseAtt,
                                                                          rateAtt, amountAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shapeAtt;

    // Matrix panel
    Panel matrixPanel { "MODULATION" };
    juce::Label  usedLabel;
    std::array<std::unique_ptr<ModSlotRow>, 8> rows;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModViewComponent)
};
