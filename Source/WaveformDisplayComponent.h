#pragma once

#include <JuceHeader.h>
#include "LookAndFeel.h"
#include "UIConstants.h"
#include <functional>
#include <vector>

class WaveformDisplayComponent : public juce::Component
{
public:
    std::function<void(std::vector<float>&)> getDisplayFrame;

    WaveformDisplayComponent() : cachedFrame (2048, 0.0f) {}

    // Call this before repaint() to refresh the cached frame without allocating in paint().
    void updateFrame()
    {
        if (getDisplayFrame)
            getDisplayFrame (cachedFrame);
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
        {
            const auto& frame = cachedFrame;
            if (! frame.empty())
            {
                const float x0 = (float) drawArea.getX();
                const float y0 = (float) drawArea.getY();
                const float w  = (float) drawArea.getWidth();
                const float h  = (float) drawArea.getHeight();
                const float cy = y0 + h * 0.5f;
                const float amp = h * 0.45f;

                const int n    = (int) frame.size();
                const int numPx = juce::jmax (2, (int) w);

                wavePath.clear();
                fillPath.clear();
                wavePath.startNewSubPath (x0, cy - frame[0] * amp);
                fillPath.startNewSubPath (x0, y0 + h);
                fillPath.lineTo (x0, cy - frame[0] * amp);
                for (int px = 1; px < numPx; ++px)
                {
                    const int idx  = (int) ((float) px / (float) (numPx - 1) * (float) (n - 1) + 0.5f);
                    const float x  = x0 + (float) px / (float) (numPx - 1) * w;
                    const float py = cy - frame[(size_t) juce::jlimit (0, n - 1, idx)] * amp;
                    wavePath.lineTo (x, py);
                    fillPath.lineTo (x, py);
                }
                fillPath.lineTo (x0 + w, y0 + h);
                fillPath.closeSubPath();
                g.setColour (BassLookAndFeel::accent.withAlpha (UIConst::displayFillAlpha));
                g.fillPath (fillPath);
                g.setColour (BassLookAndFeel::accent);
                g.strokePath (wavePath, juce::PathStrokeType (UIConst::displayStrokeWidth));
            }
        }
    }

private:
    std::vector<float> cachedFrame;
    juce::Path wavePath, fillPath;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveformDisplayComponent)
};
