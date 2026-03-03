#pragma once

#include <JuceHeader.h>
#include <array>

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
        juce::ComboBox sourceCombo, targetCombo;
        juce::Slider   amountSlider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> sourceAttach, targetAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   amountAttach;
    };

    std::array<SlotRow, 8> rows_;

    static constexpr int kRowH    = 34;
    static constexpr int kHeaderH = 24;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModMatrixComponent)
};
