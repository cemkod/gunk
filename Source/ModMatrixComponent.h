#pragma once

#include <JuceHeader.h>
#include <array>
#include "ModMatrix.h"

class TargetComboBox : public juce::ComboBox
{
public:
    void showPopup() override
    {
        juce::PopupMenu menu;
        menu.addItem ((int) ModTarget::None, "None");
        menu.addSeparator();

        juce::PopupMenu osc1;
        osc1.addItem ((int) ModTarget::Osc1Level,    "Level");
        osc1.addItem ((int) ModTarget::Morph1,        "Morph");
        osc1.addItem ((int) ModTarget::Osc1FineTune,  "Fine Tune");
        osc1.addItem ((int) ModTarget::Unison1Detune, "Unison Detune");
        osc1.addItem ((int) ModTarget::Unison1Blend,  "Unison Blend");
        menu.addSubMenu ("OSC 1", osc1);

        juce::PopupMenu osc2;
        osc2.addItem ((int) ModTarget::Osc2Level,    "Level");
        osc2.addItem ((int) ModTarget::Morph2,        "Morph");
        osc2.addItem ((int) ModTarget::Osc2FineTune,  "Fine Tune");
        osc2.addItem ((int) ModTarget::Unison2Detune, "Unison Detune");
        osc2.addItem ((int) ModTarget::Unison2Blend,  "Unison Blend");
        menu.addSubMenu ("OSC 2", osc2);

        juce::PopupMenu filter;
        filter.addItem ((int) ModTarget::FilterFreq, "Frequency");
        filter.addItem ((int) ModTarget::FilterRes,  "Resonance");
        menu.addSubMenu ("Filter", filter);

        juce::PopupMenu lfo;
        lfo.addItem ((int) ModTarget::LfoRate,   "Rate");
        lfo.addItem ((int) ModTarget::LfoAmount, "Amount");
        menu.addSubMenu ("LFO", lfo);

        juce::PopupMenu global;
        global.addItem ((int) ModTarget::Glide,       "Glide");
        global.addItem ((int) ModTarget::SubLevel,     "Sub Level");
        global.addItem ((int) ModTarget::MasterVolume, "Master Volume");
        menu.addSubMenu ("Global", global);

        menu.showMenuAsync (
            juce::PopupMenu::Options().withTargetScreenArea (getScreenBounds()),
            [this] (int result) {
                if (result != 0)
                    setSelectedId (result, juce::sendNotification);
                hidePopup(); // reset menuActive so the popup can be opened again
            });
    }
};

class ModMatrixComponent : public juce::Component
{
public:
    explicit ModMatrixComponent (juce::AudioProcessorValueTreeState& apvts);
    ~ModMatrixComponent() override;

    void paint  (juce::Graphics& g) override;
    void resized() override;

private:
    struct SlotRow
    {
        juce::ComboBox   sourceCombo;
        TargetComboBox   targetCombo;
        juce::Slider   amountSlider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> sourceAttach, targetAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   amountAttach;
    };

    std::array<SlotRow, 8> rows_;

    static constexpr int kRowH    = 34;
    static constexpr int kHeaderH = 24;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModMatrixComponent)
};
