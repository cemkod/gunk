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
        g.fillRoundedRectangle (bounds.toFloat(), UIConst::displayCornerRadius);
        g.setColour (BassLookAndFeel::border);
        g.drawRoundedRectangle (bounds.toFloat(), UIConst::displayCornerRadius, 1.0f);

        auto drawArea = bounds.reduced (1);

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
                juce::Path path, fill;
                path.startNewSubPath (x0, cy - frame[0] * amp);
                fill.startNewSubPath (x0, y0 + h);
                fill.lineTo (x0, cy - frame[0] * amp);
                for (int i = 1; i < n; ++i)
                {
                    const float x = x0 + (float) i / (float) (n - 1) * w;
                    const float py = cy - frame[(size_t) i] * amp;
                    path.lineTo (x, py);
                    fill.lineTo (x, py);
                }
                fill.lineTo (x0 + w, y0 + h);
                fill.closeSubPath();
                g.setColour (BassLookAndFeel::accent.withAlpha (UIConst::displayFillAlpha));
                g.fillPath (fill);
                g.setColour (BassLookAndFeel::accent);
                g.strokePath (path, juce::PathStrokeType (UIConst::displayStrokeWidth));
            }
        }
    }

private:
    void timerCallback() override { repaint(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveformDisplayComponent)
};
