#pragma once

#include <JuceHeader.h>
#include "LookAndFeel.h"
#include "UIConstants.h"
#include <functional>
#include <vector>

class WaveformDisplayComponent : public juce::Component,
                                 private juce::Timer
{
public:
    std::function<std::vector<float>()> getDisplayFrame;
    std::function<juce::String()>       getWavetableName;
    std::function<void()>               onMouseDown;

    WaveformDisplayComponent()
    {
        startTimerHz (15);
    }

    ~WaveformDisplayComponent() override
    {
        stopTimer();
    }

    void paint (juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds();

        // Background + border
        g.setColour (BassLookAndFeel::surfaceDark);
        g.fillRoundedRectangle (bounds.toFloat(), 3.0f);
        g.setColour (BassLookAndFeel::border);
        g.drawRoundedRectangle (bounds.toFloat(), 3.0f, 1.0f);

        // Info bar at bottom
        constexpr int infoBarH = 14;
        auto drawArea = bounds.reduced (1);
        auto infoArea = drawArea.removeFromBottom (infoBarH);

        juce::String name = getWavetableName ? getWavetableName() : juce::String{};
        g.setFont (juce::Font (UIConst::uiFontSize));
        g.setColour (BassLookAndFeel::textDim);
        g.drawText (name.isEmpty() ? "click to select" : name,
                    infoArea.reduced (4, 0), juce::Justification::centredLeft);

        // Waveform path
        if (getDisplayFrame)
        {
            const auto frame = getDisplayFrame();
            if (! frame.empty())
            {
                const float x0 = (float) drawArea.getX();
                const float y0 = (float) drawArea.getY();
                const float w  = (float) drawArea.getWidth();
                const float h  = (float) drawArea.getHeight();
                const float cy = y0 + h * 0.5f;
                const float amp = h * 0.45f;

                const int n = (int) frame.size();
                juce::Path path;
                path.startNewSubPath (x0, cy - frame[0] * amp);
                for (int i = 1; i < n; ++i)
                {
                    const float x = x0 + (float) i / (float) (n - 1) * w;
                    path.lineTo (x, cy - frame[(size_t) i] * amp);
                }
                g.setColour (BassLookAndFeel::accent);
                g.strokePath (path, juce::PathStrokeType (1.5f));
            }
        }
    }

    void mouseDown (const juce::MouseEvent&) override
    {
        if (onMouseDown) onMouseDown();
    }

private:
    void timerCallback() override { repaint(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveformDisplayComponent)
};
