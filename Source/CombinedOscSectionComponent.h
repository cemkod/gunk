#pragma once

#include <JuceHeader.h>
#include "LabelledSectionComponent.h"
#include "OscSectionComponent.h"
#include "WaveformDisplayComponent.h"
#include <functional>
#include <array>
#include <vector>

class CombinedOscSectionComponent : public LabelledSectionComponent
{
public:
    // Modulated parameter getters — set from PluginEditor
    std::function<float(int)> getModDetune;  // oscIdx → modulated detune cents
    std::function<float(int)> getModBlend;   // oscIdx → modulated blend 0..1

    // Per-oscillator callbacks (index 0 = OSC 1, index 1 = OSC 2)
    std::array<std::function<bool()>, 2>                  isCustomWavetableLoaded;
    std::array<std::function<bool()>, 2>                  isCustomWaveformActive;
    std::array<std::function<void()>, 2>                  reactivateCustomWavetable;
    std::array<std::function<bool(const juce::File&)>, 2> loadWavetableFromFile;
    std::array<std::function<int()>, 2>                   getNumFrames;
    std::array<std::function<std::vector<float>()>, 2>    getDisplayFrame;
    std::array<std::function<juce::String()>, 2>          getWavetableName;

    CombinedOscSectionComponent (juce::AudioProcessorValueTreeState& apvts,
                                  const OscParamIds& ids1,
                                  const OscParamIds& ids2);
    ~CombinedOscSectionComponent() override;

    void resized() override;
    void updateButtonStates();

private:
    juce::AudioProcessorValueTreeState& apvts;
    OscParamIds paramIds[2];

    juce::TextButton tabBtn1 { "1" }, tabBtn2 { "2" };
    WaveformDisplayComponent waveformDisplay;

    // Unison spread display — right side of waveform row
    struct UnisonDisplayComponent : public juce::Component, private juce::Timer
    {
        std::function<float()> getVoices;
        std::function<float()> getDetune;
        std::function<float()> getBlend;

        UnisonDisplayComponent()  { startTimerHz (15); }
        ~UnisonDisplayComponent() { stopTimer(); }

        void timerCallback() override { repaint(); }

        void paint (juce::Graphics& g) override
        {
            const auto b = getLocalBounds().toFloat();
            g.setColour (BassLookAndFeel::surfaceDark);
            g.fillRoundedRectangle (b, UIConst::displayCornerRadius);
            g.setColour (BassLookAndFeel::border);
            g.drawRoundedRectangle (b.reduced (0.5f), UIConst::displayCornerRadius, 1.0f);

            const int voices = juce::roundToInt (getVoices ? getVoices() : 1.0f);
            const float detune = getDetune ? getDetune() : 0.0f;
            const float blend  = getBlend  ? getBlend()  : 1.0f;

            const float spreadFrac = detune / 100.0f;
            const float cx  = b.getCentreX();
            const float cy  = b.getCentreY();
            const float maxHalfH = b.getHeight() * 0.35f;
            const float halfW    = b.getWidth() * 0.5f - 4.0f;

            // Horizontal axis
            g.setColour (BassLookAndFeel::border);
            g.drawHorizontalLine ((int) cy, b.getX() + 4.0f, b.getRight() - 4.0f);

            for (int i = 0; i < voices; ++i)
            {
                float t = (voices == 1) ? 0.0f
                                        : (float) i / (float) (voices - 1) * 2.0f - 1.0f; // -1..+1

                const float x = cx + t * spreadFrac * halfW;

                // Line height: centre voice full, outer voices scale with blend
                const float abst     = std::abs (t);
                const float halfH    = maxHalfH * (1.0f - abst * (1.0f - blend));

                // Colour: centre = accent, edges fade toward iconDim
                const float alpha = 1.0f - abst * 0.65f;
                const auto col = BassLookAndFeel::accent.withAlpha (alpha)
                                     .interpolatedWith (BassLookAndFeel::iconDim, abst * 0.4f);

                g.setColour (col);
                g.drawLine (x, cy - halfH, x, cy + halfH, 1.5f);
            }
        }
    } unisonDisplay;
    OscSectionComponent osc1, osc2Comp;

    juce::ComboBox waveCombo1, waveCombo2;
    juce::ComboBox octCombo1, octCombo2;
    juce::Label    octLabel;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment octAttach1, octAttach2;

    int selectedOsc = 0;

    std::unique_ptr<juce::FileChooser> fileChooser;

    void selectOsc (int idx);
    void setWaveformParam (int oscIdx, int waveIdx);
    void openWavFileDialog (int oscIdx);
    void syncWaveCombo (int oscIdx);
    void setupWaveCombo (juce::ComboBox& combo, int oscIdx);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CombinedOscSectionComponent)
};
