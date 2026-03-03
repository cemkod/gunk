#pragma once

#include <JuceHeader.h>
#include "LookAndFeel.h"

class EnvelopeDisplayComponent : public juce::Component
{
public:
    EnvelopeDisplayComponent() : history (256, 0.0f) {}

    void pushSample (float v)
    {
        history[(size_t) writePos] = v;
        writePos = (writePos + 1) % (int) history.size();
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        g.setColour (BassLookAndFeel::surfaceDark);
        g.fillRoundedRectangle (bounds, 4.0f);
        g.setColour (BassLookAndFeel::border);
        g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.0f);

        const int N = (int) history.size();
        const float w = bounds.getWidth();
        const float h = bounds.getHeight();
        const float x0 = bounds.getX();
        const float y0 = bounds.getY();

        // Map 0..1 → bottom..top with a dB-like curve for visual clarity
        auto toY = [&] (float v) -> float {
            const float dB   = 20.0f * std::log10 (juce::jmax (v, 1e-10f));
            const float norm = juce::jlimit (0.0f, 1.0f, (dB - (-60.0f)) / 60.0f);
            return y0 + h * (1.0f - norm);
        };

        const juce::Colour lineColour { 0xff4fcfff };

        juce::Path wavePath, fillPath;
        bool pathStarted = false;

        for (int i = 0; i < N; ++i)
        {
            const int idx = (writePos + i) % N;
            const float px = x0 + (float) i / (float) (N - 1) * w;
            const float py = toY (history[(size_t) idx]);

            if (!pathStarted)
            {
                wavePath.startNewSubPath (px, py);
                fillPath.startNewSubPath (px, y0 + h);
                fillPath.lineTo (px, py);
                pathStarted = true;
            }
            else
            {
                wavePath.lineTo (px, py);
                fillPath.lineTo (px, py);
            }
        }

        if (pathStarted)
        {
            fillPath.lineTo (x0 + w, y0 + h);
            fillPath.closeSubPath();

            g.setColour (lineColour.withAlpha (0.12f));
            g.fillPath (fillPath);

            g.setColour (lineColour);
            g.strokePath (wavePath, juce::PathStrokeType (1.5f));
        }
    }

private:
    std::vector<float> history;
    int writePos = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EnvelopeDisplayComponent)
};
