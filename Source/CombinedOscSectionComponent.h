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
    OscSectionComponent osc1, osc2Comp;

    int selectedOsc = 0;

    std::unique_ptr<juce::FileChooser> fileChooser;

    void selectOsc (int idx);
    void showWaveformMenu();
    void setWaveformParam (int oscIdx, int waveIdx);
    void openWavFileDialog (int oscIdx);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CombinedOscSectionComponent)
};
