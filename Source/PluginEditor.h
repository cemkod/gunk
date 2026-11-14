#pragma once

#include "PluginProcessor.h"
#include "LookAndFeel.h"
#include "GateSectionComponent.h"
#include "CombinedOscSectionComponent.h"
#include "FilterSectionComponent.h"
#include "SubOscSectionComponent.h"
#include "TransientSectionComponent.h"
#include "EnvelopeSectionComponent.h"
#include "ModMatrixComponent.h"
#include "OutputSectionComponent.h"
#include "IRSectionComponent.h"
#include "DriveSectionComponent.h"
#include "LFOSectionComponent.h"

class JQGunkAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       public juce::Timer
{
public:
    explicit JQGunkAudioProcessorEditor (JQGunkAudioProcessor&);
    ~JQGunkAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;

private:
    JQGunkAudioProcessor& processor;

    BassLookAndFeel lookAndFeel;

    GateSectionComponent          gateSection;
    EnvelopeSectionComponent      envelopeSection;
    FilterSectionComponent        filterSection;
    SubOscSectionComponent        subOscSection;
    CombinedOscSectionComponent   combinedOscSection;
    TransientSectionComponent     transientSection;
    IRSectionComponent            irSection;
    DriveSectionComponent         driveSection;
    OutputSectionComponent        outputSection;
    LFOSectionComponent           lfoSection;
    ModMatrixComponent            modMatrixView;
    juce::TextButton              modToggleBtn { "MOD" };
    bool modViewVisible = false;

    std::unique_ptr<juce::Drawable> logo;

    juce::Label freqLabel;
    bool gateOpen = false;
    juce::Rectangle<int> gateLedBounds;

    bool transientLedActive = false;
    juce::Rectangle<int> transientLedBounds;

    // Preset strip
    juce::ComboBox   presetCombo;
    juce::TextButton presetSaveBtn  { "Save" };
    juce::TextButton presetDeleteBtn { "Del" };
    juce::Rectangle<int> presetStripBounds;

    void refreshPresetCombo();
    void onSavePreset();

    void timerCallback() override;

#ifdef ENABLE_DEBUG_LOG
    struct SectionTiming
    {
        juce::String name;
        double totalMs = 0.0, maxMs = 0.0;
        int count = 0;
        void record (double ms) { totalMs += ms; if (ms > maxMs) maxMs = ms; ++count; }
        juce::String report() const
        {
            if (count == 0) return name + ": no data";
            return name + " avg=" + juce::String (totalMs / count, 3)
                        + "ms max=" + juce::String (maxMs, 3) + "ms";
        }
        void reset() { totalMs = 0.0; maxMs = 0.0; count = 0; }
    };
    SectionTiming stWaveform { "waveform" }, stFilter { "filter" },
                  stGate     { "gate"     }, stEnvelope { "envelope" },
                  stLFO      { "lfo"      }, stTotal { "total" };
    double timerLastLogMs = -1.0;
    static constexpr double kTimerLogIntervalMs = 30'000.0;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JQGunkAudioProcessorEditor)
};
