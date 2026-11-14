#pragma once

#include <JuceHeader.h>
#include "SectionPanel.h"
#include "OscSectionComponent.h"
#include "PowerToggleButton.h"
#include "WaveformDisplayComponent.h"
#include <functional>
#include <array>
#include <vector>

// OSCILLATORS: header tabs OSC 1 | OSC 2 | SUB, each with its own power switch.
// The OSC pages show the waveform, a unison spread display and two knob rows;
// the SUB page shows a sine, the octave, the skip-filter switch and the level.
class CombinedOscSectionComponent : public SectionPanel
{
public:
    // Modulated parameter getters, set from the main view
    std::function<float(int)> getModDetune;  // oscIdx -> modulated detune cents
    std::function<float(int)> getModBlend;   // oscIdx -> modulated blend 0..1
    std::function<float(int)> getModWidth;   // oscIdx -> modulated stereo width 0..1

    // Per-oscillator callbacks (index 0 = OSC 1, index 1 = OSC 2)
    std::array<std::function<bool()>, 2>                  isCustomWavetableLoaded;
    std::array<std::function<bool()>, 2>                  isCustomWaveformActive;
    std::array<std::function<void()>, 2>                  reactivateCustomWavetable;
    std::array<std::function<bool(const juce::File&)>, 2> loadWavetableFromFile;
    std::array<std::function<int()>, 2>                   getNumFrames;
    std::array<std::function<void(std::vector<float>&)>, 2> getDisplayFrame;
    std::array<std::function<juce::String()>, 2>          getWavetableName;

    CombinedOscSectionComponent (juce::AudioProcessorValueTreeState& apvts,
                                  const OscParamIds& ids1,
                                  const OscParamIds& ids2);

    void updateButtonStates();
    void repaintDisplays() { waveformDisplay.updateFrame(); waveformDisplay.repaint(); unisonDisplay.repaint(); }
    void refreshDimmed() override;

    void paint (juce::Graphics&) override;

    static constexpr int kSubTab = 2;

private:
    void layoutBody (juce::Rectangle<int> body) override;
    bool isHeaderChild (juce::Component* c) const override
    {
        return c == &tabs[0] || c == &tabs[1] || c == &tabs[2];
    }

    juce::AudioProcessorValueTreeState& apvts;
    OscParamIds paramIds[2];

    // Header tab: click the name to select, click the switch to enable/bypass.
    struct OscTab : public juce::Component
    {
        explicit OscTab (juce::String n) : name (std::move (n))
        {
            power.setClickingTogglesState (true);
            addAndMakeVisible (power);
            setMouseCursor (juce::MouseCursor::PointingHandCursor);
        }
        void paint (juce::Graphics& g) override
        {
            const auto b = getLocalBounds().toFloat().reduced (0.5f);
            g.setColour (selected ? BassLookAndFeel::surfaceDark : BassLookAndFeel::surfaceDark.withAlpha (0.5f));
            g.fillRoundedRectangle (b, UIConst::buttonCornerRadius);
            g.setColour (selected ? BassLookAndFeel::accent : BassLookAndFeel::border);
            g.drawRoundedRectangle (b, UIConst::buttonCornerRadius, selected ? 1.5f : 1.0f);
            g.setColour (selected ? BassLookAndFeel::accent
                                  : (isMouseOver() ? BassLookAndFeel::text : BassLookAndFeel::textDim));
            g.setFont (BassLookAndFeel::uiFont (UIConst::fontButtonText, true));
            g.drawText (name, getLocalBounds().withTrimmedLeft (10).withTrimmedRight (UIConst::powerToggleW + 8),
                        juce::Justification::centredLeft);
        }
        void resized() override
        {
            power.setBounds (getLocalBounds().removeFromRight (UIConst::powerToggleW + 8).withTrimmedRight (8)
                                             .withSizeKeepingCentre (UIConst::powerToggleW, UIConst::powerToggleH));
        }
        void mouseUp (const juce::MouseEvent& e) override { if (e.mouseWasClicked() && onSelect) onSelect(); }
        void mouseEnter (const juce::MouseEvent&) override { repaint(); }
        void mouseExit  (const juce::MouseEvent&) override { repaint(); }

        juce::String name;
        PowerToggleButton power;
        bool selected = false;
        std::function<void()> onSelect;
    };
    std::array<OscTab, 3> tabs { OscTab ("OSC 1"), OscTab ("OSC 2"), OscTab ("SUB") };
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>, 3> tabPowerAttach;
    std::array<std::atomic<float>*, 3> enabledParams {};

    WaveformDisplayComponent waveformDisplay;

    // Unison voices placed by their stereo position (detune position x width, as the DSP
    // pans them); line height follows blend.
    struct UnisonDisplayComponent : public juce::Component
    {
        std::function<float()> getVoices, getBlend, getWidth;

        void paint (juce::Graphics& g) override
        {
            const auto b = getLocalBounds().toFloat();
            g.setColour (BassLookAndFeel::surfaceDark);
            g.fillRoundedRectangle (b, UIConst::displayCornerRadius);
            g.setColour (BassLookAndFeel::border);
            g.drawRoundedRectangle (b.reduced (0.5f), UIConst::displayCornerRadius, 1.0f);

            const int   voices = juce::roundToInt (getVoices ? getVoices() : 1.0f);
            const float blend  = getBlend ? getBlend() : 1.0f;
            const float width  = getWidth ? getWidth() : 0.0f;

            const float cx = b.getCentreX();
            const float cy = b.getCentreY() - 6.0f;
            const float maxHalfH = b.getHeight() * 0.32f;
            const float halfW    = b.getWidth() * 0.5f - 18.0f;

            g.setColour (BassLookAndFeel::border);
            g.drawHorizontalLine ((int) cy, b.getX() + 8.0f, b.getRight() - 8.0f);
            g.setColour (BassLookAndFeel::textDim);
            g.setFont (BassLookAndFeel::uiFont (UIConst::fontCaption, true));
            g.drawText ("L", b.reduced (8.0f, 4.0f), juce::Justification::bottomLeft);
            g.drawText ("R", b.reduced (8.0f, 4.0f), juce::Justification::bottomRight);

            // Draw outer voices first so the centre one sits on top
            for (int pass = 0; pass < 2; ++pass)
                for (int i = 0; i < voices; ++i)
                {
                    const float t    = (voices == 1) ? 0.0f : (float) i / (float) (voices - 1) * 2.0f - 1.0f;
                    const float abst = std::abs (t);
                    if ((pass == 0) != (abst > 0.01f)) continue;

                    const float x     = cx + t * width * halfW;
                    const float halfH = maxHalfH * (1.0f - abst * (1.0f - blend));
                    const auto col = BassLookAndFeel::accent.withAlpha (1.0f - abst * 0.5f);
                    g.setColour (col);
                    g.drawLine (x, cy - halfH, x, cy + halfH, 2.0f);
                }
        }
    } unisonDisplay;

    OscSectionComponent osc1, osc2Comp;

    juce::ComboBox waveCombo1, waveCombo2;
    juce::ComboBox octCombo1, octCombo2;
    juce::Label    octLabel, unisonCaption;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> octAttach1, octAttach2;

    // SUB page
    juce::Slider     subLevelSlider;
    juce::Label      subLevelLabel, subOctLabel;
    juce::ComboBox   subOctCombo;
    juce::TextButton skipFilterBtn { "SKIP FILTER" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   subLevelAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> subOctAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   skipFilterAttach;

    int selectedTab = 0;
    int separatorX = -1;

    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::Array<juce::File> factoryWavetables;

    void selectTab (int idx);
    void setWaveformParam (int oscIdx, int waveIdx);
    void openWavFileDialog (int oscIdx);
    void syncWaveCombo (int oscIdx);
    void setupWaveCombo (juce::ComboBox& combo, int oscIdx);
    void addFactorySubmenu (juce::ComboBox& combo);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CombinedOscSectionComponent)
};
