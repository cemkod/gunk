#pragma once

#include <JuceHeader.h>
#include "SectionPanel.h"
#include "LookAndFeel.h"
#include "UIConstants.h"
#include "PowerToggleButton.h"
#include "ParameterIDs.h"

// TRANSIENT: a sample played on each detected onset. The display well shows the loaded
// sample's waveform and name; click it to browse, or drop an audio file on it.
class TransientSectionComponent : public SectionPanel
{
public:
    // Wired by the editor after construction
    std::function<bool(const juce::File&)> loadSampleFromFile;
    std::function<juce::String()>          getLoadedPath;

    TransientSectionComponent (juce::AudioProcessorValueTreeState& avts,
                               const juce::File& libraryDir)
        : SectionPanel ("TRANSIENT"),
          pitchAttach  (avts, "transientPitch",  pitchSlider),
          levelAttach  (avts, "transientLevel",  levelSlider),
          dryAttach    (avts, "transientDry",    drySlider),
          attackAttach (avts, "transientAttack", attackSlider),
          holdAttach   (avts, "transientHold",   holdSlider),
          decayAttach  (avts, "transientDecay",  decaySlider),
          sampleLibraryDir (libraryDir),
          pwrAttach    (avts, ParamIDs::transientEnabled, pwrBtn)
    {
        pwrBtn.setClickingTogglesState (true);
        addAndMakeVisible (pwrBtn);
        setHeaderPower (&pwrBtn);
        setEnabledParameter (avts, ParamIDs::transientEnabled);

        BassLookAndFeel::setupRotarySlider (pitchSlider,  pitchLabel,  "PITCH",  *this);
        BassLookAndFeel::setupRotarySlider (levelSlider,  levelLabel,  "LEVEL",  *this);
        BassLookAndFeel::setupRotarySlider (drySlider,    dryLabel,    "DRY",    *this);
        BassLookAndFeel::setupRotarySlider (attackSlider, attackLabel, "ATTACK", *this);
        BassLookAndFeel::setupRotarySlider (holdSlider,   holdLabel,   "HOLD",   *this);
        BassLookAndFeel::setupRotarySlider (decaySlider,  decayLabel,  "DECAY",  *this);
        drySlider.setTooltip ("How much of the input passes through while the transient plays");

        sampleWell.onClick = [this] { openFileChooser(); };
        sampleWell.onFileDropped = [this] (const juce::File& f)
        {
            if (loadSampleFromFile && loadSampleFromFile (f))
                updateButtonStates();
        };
        addAndMakeVisible (sampleWell);

        updateButtonStates();
    }

    void updateButtonStates()
    {
        const juce::String path = getLoadedPath ? getLoadedPath() : juce::String();
        if (path != loadedPath)
        {
            loadedPath = path;
            sampleWell.setFile (path.isNotEmpty() ? juce::File (path) : juce::File());
        }
    }

private:
    void layoutBody (juce::Rectangle<int> body) override
    {
        using K = BassLookAndFeel::KnobSize;
        const int cellH = BassLookAndFeel::knobCellHeight (K::small);
        auto row2 = body.removeFromBottom (cellH);
        auto row1 = body.removeFromBottom (cellH);
        body.removeFromBottom (UIConst::knobGap);
        sampleWell.setBounds (body);

        BassLookAndFeel::placeKnobRow (row1, { { &pitchSlider, &pitchLabel },
                                               { &levelSlider, &levelLabel },
                                               { &drySlider,   &dryLabel } }, 0, K::small);
        BassLookAndFeel::placeKnobRow (row2, { { &attackSlider, &attackLabel },
                                               { &holdSlider,   &holdLabel },
                                               { &decaySlider,  &decayLabel } }, 0, K::small);
    }

    void openFileChooser()
    {
        fileChooser = std::make_unique<juce::FileChooser> (
            "Load Transient Sample",
            sampleLibraryDir.isDirectory() ? sampleLibraryDir
                                           : juce::File::getSpecialLocation (juce::File::userHomeDirectory),
            "*.wav;*.aif;*.aiff");

        fileChooser->launchAsync (
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this] (const juce::FileChooser& fc)
            {
                const auto results = fc.getResults();
                if (results.isEmpty()) return;
                if (loadSampleFromFile && loadSampleFromFile (results[0]))
                    updateButtonStates();
            });
    }

    // Display well: the sample's peak outline with its name, or "DROP SAMPLE".
    struct SampleWell : public juce::Component,
                        public juce::FileDragAndDropTarget,
                        public juce::SettableTooltipClient
    {
        std::function<void()> onClick;
        std::function<void (const juce::File&)> onFileDropped;

        SampleWell()
        {
            setMouseCursor (juce::MouseCursor::PointingHandCursor);
            setTooltip ("Click to load a sample, or drop a WAV/AIFF here");
        }

        // Reads the file once and keeps a small peak outline (transient samples are short).
        void setFile (const juce::File& f)
        {
            name = f.existsAsFile() ? f.getFileNameWithoutExtension() : juce::String();
            peaks.clear();
            if (f.existsAsFile())
            {
                juce::AudioFormatManager fm;
                fm.registerBasicFormats();
                if (std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (f)); r != nullptr)
                {
                    const auto len = (int) std::min<juce::int64> (r->lengthInSamples, (juce::int64) 48000 * 4);
                    juce::AudioBuffer<float> buf ((int) r->numChannels, juce::jmax (1, len));
                    r->read (&buf, 0, len, 0, true, true);
                    constexpr int kBins = 200;
                    peaks.resize (kBins, 0.0f);
                    float maxPeak = 1.0e-6f;
                    for (int b = 0; b < kBins; ++b)
                    {
                        const int s0 = len * b / kBins, s1 = juce::jmax (s0 + 1, len * (b + 1) / kBins);
                        float m = 0.0f;
                        for (int ch = 0; ch < buf.getNumChannels(); ++ch)
                            m = juce::jmax (m, buf.getMagnitude (ch, s0, s1 - s0));
                        peaks[(size_t) b] = m;
                        maxPeak = juce::jmax (maxPeak, m);
                    }
                    for (auto& p : peaks) p /= maxPeak;
                }
            }
            repaint();
        }

        void paint (juce::Graphics& g) override
        {
            const auto b = getLocalBounds().toFloat();
            g.setColour (BassLookAndFeel::surfaceDark);
            g.fillRoundedRectangle (b, UIConst::displayCornerRadius);
            g.setColour (dragOver ? BassLookAndFeel::accent : BassLookAndFeel::border);
            g.drawRoundedRectangle (b.reduced (0.5f), UIConst::displayCornerRadius, dragOver ? 1.5f : 1.0f);

            if (! peaks.empty())
            {
                const auto area = b.reduced (6.0f, 4.0f);
                const float cy = area.getCentreY(), halfH = area.getHeight() * 0.45f;
                juce::Path p;
                const int n = (int) peaks.size();
                for (int i = 0; i < n; ++i)
                {
                    const float x = area.getX() + area.getWidth() * (float) i / (float) (n - 1);
                    const float y = cy - peaks[(size_t) i] * halfH;
                    if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
                }
                for (int i = n - 1; i >= 0; --i)
                    p.lineTo (area.getX() + area.getWidth() * (float) i / (float) (n - 1), cy + peaks[(size_t) i] * halfH);
                p.closeSubPath();
                g.setColour (BassLookAndFeel::accent.withAlpha (0.25f));
                g.fillPath (p);
                g.setColour (BassLookAndFeel::accent.withAlpha (0.8f));
                g.strokePath (p, juce::PathStrokeType (1.0f));
            }

            g.setFont (BassLookAndFeel::uiFont (UIConst::fontButtonText, true));
            if (name.isNotEmpty())
            {
                g.setColour (BassLookAndFeel::text);
                g.drawFittedText (name, getLocalBounds().reduced (8, 4), juce::Justification::bottomRight, 1);
            }
            else
            {
                g.setColour (isMouseOver() ? BassLookAndFeel::accent : BassLookAndFeel::textDim);
                g.drawText ("DROP SAMPLE", getLocalBounds(), juce::Justification::centred);
            }
        }

        void mouseUp (const juce::MouseEvent& e) override
        {
            if (e.mouseWasClicked() && onClick) onClick();
        }
        void mouseEnter (const juce::MouseEvent&) override { repaint(); }
        void mouseExit  (const juce::MouseEvent&) override { repaint(); }

        bool isInterestedInFileDrag (const juce::StringArray& files) override
        {
            return files.size() == 1 && juce::File (files[0]).hasFileExtension ("wav;aif;aiff");
        }
        void fileDragEnter (const juce::StringArray&, int, int) override { dragOver = true;  repaint(); }
        void fileDragExit  (const juce::StringArray&) override           { dragOver = false; repaint(); }
        void filesDropped (const juce::StringArray& files, int, int) override
        {
            dragOver = false;
            repaint();
            if (onFileDropped) onFileDropped (juce::File (files[0]));
        }

        std::vector<float> peaks;
        juce::String name;
        bool dragOver = false;
    } sampleWell;

    juce::Slider pitchSlider, levelSlider, drySlider, attackSlider, holdSlider, decaySlider;
    juce::Label  pitchLabel,  levelLabel,  dryLabel,  attackLabel,  holdLabel,  decayLabel;

    juce::AudioProcessorValueTreeState::SliderAttachment pitchAttach, levelAttach, dryAttach, attackAttach, holdAttach, decayAttach;

    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::File   sampleLibraryDir;  // files inside it are saved preset-relative
    juce::String loadedPath;

    PowerToggleButton pwrBtn;
    juce::AudioProcessorValueTreeState::ButtonAttachment pwrAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TransientSectionComponent)
};
