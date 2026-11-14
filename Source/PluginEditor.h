#pragma once

#include "PluginProcessor.h"
#include "LookAndFeel.h"
#include "TopBarComponent.h"
#include "MainViewComponent.h"
#include "ModMatrixComponent.h"

class JQGunkAudioProcessorEditor : public juce::AudioProcessorEditor,
                                   public juce::Timer
{
public:
    explicit JQGunkAudioProcessorEditor (JQGunkAudioProcessor&);
    ~JQGunkAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    JQGunkAudioProcessor& processor;

    BassLookAndFeel lookAndFeel;

    // Fixed-size design canvas (UIConst::editorWidth x editorHeight). Every child lives in
    // here, and resized() scales it to the window with a transform.
    struct Canvas : public juce::Component
    {
        std::function<void()> onResized;
        void paint (juce::Graphics& g) override { g.fillAll (BassLookAndFeel::bg); }
        void resized() override                 { if (onResized) onResized(); }
    } canvas;

    TopBarComponent    topBar;
    MainViewComponent  mainView;
    ModMatrixComponent modView;

    juce::TooltipWindow tooltipWindow { this, 600 };

    void layoutCanvas();
    void timerCallback() override;

#ifdef ENABLE_DEBUG_LOG
    double timerTotalMs = 0.0, timerMaxMs = 0.0;
    int    timerCount = 0;
    double timerLastLogMs = -1.0;
    static constexpr double kTimerLogIntervalMs = 30'000.0;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JQGunkAudioProcessorEditor)
};
