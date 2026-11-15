#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "LookAndFeel.h"
#include "IconButton.h"
#include "SegmentedControl.h"

// The bar across the top of the editor:
//   logo | < preset > save delete | note + Hz, GATE and HIT LEDs, MAIN|MOD, output meter
class TopBarComponent : public juce::Component
{
public:
    explicit TopBarComponent (JQGunkAudioProcessor& p);

    // Called with true when the MOD view is selected, false for MAIN.
    std::function<void (bool)> onViewChanged;

    // Called from the editor timer. `transientFired` is the value the editor consumed
    // from the processor this tick.
    void update (bool gateOpen, bool transientFired);

    void paint (juce::Graphics&) override;
    void resized() override;

    // Note name for a frequency, e.g. 55 Hz -> "A1"; "--" when there is no pitch.
    static juce::String noteName (float hz);

private:
    JQGunkAudioProcessor& processor;

    std::unique_ptr<juce::Drawable> logo;
    juce::Rectangle<int> logoBounds;
    juce::Rectangle<int> versionBounds;    // "v0.4.0", beside the logo lettering

    IconButton prevBtn { IconButton::Icon::prev };
    IconButton nextBtn { IconButton::Icon::next };
    juce::ComboBox presetCombo;
    IconButton saveBtn   { IconButton::Icon::save,  true };
    IconButton deleteBtn { IconButton::Icon::trash, true };
    juce::Rectangle<int> presetBoxBounds;

    SegmentedControl viewSwitch { { "MAIN", "MOD" } };

    // Readouts and meters, drawn in paint()
    juce::Rectangle<int> readoutBounds, gateLedBounds, hitLedBounds, meterBounds;
    juce::String noteText { "--" }, hzText { "-- Hz" };
    bool gateOpen = false;
    int  hitHoldTicks = 0;          // keeps the HIT LED lit for a few frames
    float meterLevel = 0.0f;        // linear, with decay

    void refreshPresetCombo();
    void loadPresetAt (int index);
    void onSavePreset();
    void updateDeleteEnabled();

    void drawLed (juce::Graphics&, juce::Rectangle<int>, bool on, juce::Colour onCol, const juce::String& label);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TopBarComponent)
};
