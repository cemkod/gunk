#pragma once

#include "PluginProcessor.h"
#include "LookAndFeel.h"
#include "GateSectionComponent.h"
#include "OscSectionComponent.h"
#include "FilterSectionComponent.h"

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

    GateSectionComponent   gateSection;
    OscSectionComponent    oscSection;
    FilterSectionComponent filterSection;

    std::unique_ptr<juce::Drawable> logo;

    juce::Label freqLabel;
    bool gateOpen = false;
    juce::Rectangle<int> gateLedBounds;

    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JQGunkAudioProcessorEditor)
};
