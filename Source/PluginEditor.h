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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JQGunkAudioProcessorEditor)
};
