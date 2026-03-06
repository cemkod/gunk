#pragma once

#include <JuceHeader.h>
#include "LabelledSectionComponent.h"
#include <functional>

struct OscParamIds
{
    juce::String waveform, level, unisonVoices, unisonDetune, unisonBlend, octaveShift, morph,
                 coarseTune, fineTune;
};

class OscSectionComponent : public LabelledSectionComponent
{
public:
    // Callback for morph enable/disable (returns oscillator frame count)
    std::function<int()> getNumFrames;

    OscSectionComponent (const juce::String& title,
                         const OscParamIds& ids,
                         juce::AudioProcessorValueTreeState& apvts,
                         bool embedded = false);
    ~OscSectionComponent() override;

    void resized() override;
    void paint (juce::Graphics& g) override
    {
        if (dividerY <= 0) return;

        const int   lineY  = dividerY + 10;   // vertically centred in rowH
        const float lx     = 0.0f;
        const float lw     = (float) getWidth();

        // Full-width divider line
        g.setColour (BassLookAndFeel::border);
        g.fillRect (lx, (float) lineY, lw, 1.0f);

        // Text with background cutout so line doesn't show through
        juce::Font font (UIConst::fontSectionTitle, juce::Font::bold);
        g.setFont (font);
        const juce::String txt = "UNISON";
        const float tw  = font.getStringWidthFloat (txt) + 6.0f;
        const float tx  = 0.0f;
        const float th  = (float) 20;
        const float ty  = (float) dividerY;

        g.setColour (BassLookAndFeel::surface);
        g.fillRect (tx, ty, tw, th);

        g.setColour (BassLookAndFeel::textDim);
        g.drawText (txt, (int) tx, (int) ty, (int) tw, (int) th, juce::Justification::centredLeft);
    }

    void updateButtonStates();

private:
    OscParamIds paramIds;
    juce::AudioProcessorValueTreeState& apvts;
    bool embedded_;

    juce::Slider levelSlider;
    juce::Label  levelLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment levelAttach;

    juce::Slider unisonVoicesSlider, unisonDetuneSlider, unisonBlendSlider;
    juce::Label  unisonVoicesLabel,  unisonDetuneLabel,  unisonBlendLabel;
    int dividerY = 0;
    juce::AudioProcessorValueTreeState::SliderAttachment unisonVoicesAttach,
                                                          unisonDetuneAttach,
                                                          unisonBlendAttach;

    juce::Slider morphSlider;
    juce::Label  morphLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment morphAttach;

    juce::Slider coarseTuneSlider, fineTuneSlider;
    juce::Label  coarseTuneLabel,  fineTuneLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment coarseTuneAttach, fineTuneAttach;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OscSectionComponent)
};
