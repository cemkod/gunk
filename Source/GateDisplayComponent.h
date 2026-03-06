#pragma once

#include <JuceHeader.h>
#include "LookAndFeel.h"
#include "ParameterIDs.h"

class GateDisplayComponent : public juce::Component
{
public:
    GateDisplayComponent (juce::AudioProcessorValueTreeState& avts)
        : apvts (avts), history (256, 0.0f), transientHistory (256, false)
    {}

    void pushSample (float envelope, bool gateOpen, bool transientFired = false)
    {
        history[(size_t) writePos] = envelope;
        transientHistory[(size_t) writePos] = transientFired;
        writePos = (writePos + 1) % (int) history.size();
        lastGateOpen = gateOpen;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Background
        g.setColour (BassLookAndFeel::surfaceDark);
        g.fillRoundedRectangle (bounds, 4.0f);
        g.setColour (BassLookAndFeel::border);
        g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.0f);

        const int N = (int) history.size();
        const float w = bounds.getWidth();
        const float h = bounds.getHeight();
        const float x0 = bounds.getX();
        const float y0 = bounds.getY();

        // dB-scale helper: maps linear amplitude to 0..1 (−60..0 dBFS)
        auto toY = [&] (float linear) -> float {
            const float dB = 20.0f * std::log10 (juce::jmax (linear, 1e-10f));
            const float norm = juce::jlimit (0.0f, 1.0f, (dB - (-60.0f)) / 60.0f);
            return y0 + h * (1.0f - norm);
        };

        // Read threshold params
        const float thresh = *apvts.getRawParameterValue (ParamIDs::gateThreshold);
        const float hyst   = *apvts.getRawParameterValue (ParamIDs::gateHysteresis);

        // Hysteresis open threshold line (amber, dimmer)
        const float openThresh = thresh * std::pow (10.0f, hyst / 20.0f);
        const float openY = toY (openThresh);
        g.setColour (juce::Colour (0xffffaa44).withAlpha (0.5f));
        g.drawHorizontalLine (juce::roundToInt (openY), x0 + 1.0f, x0 + w - 1.0f);

        // Close threshold line (orange-red)
        const float threshY = toY (thresh);
        g.setColour (juce::Colour (0xffff6633).withAlpha (0.85f));
        g.drawHorizontalLine (juce::roundToInt (threshY), x0 + 1.0f, x0 + w - 1.0f);

        // Build waveform path
        const juce::Colour lineColour = lastGateOpen
            ? BassLookAndFeel::accent
            : BassLookAndFeel::accent.withAlpha (0.35f);

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

        // Transient trigger markers: faint amber vertical lines
        g.setColour (juce::Colour (0xffffaa44).withAlpha (0.4f));
        for (int i = 0; i < N; ++i)
        {
            const int idx = (writePos + i) % N;
            if (transientHistory[(size_t) idx])
            {
                const float px = x0 + (float) i / (float) (N - 1) * w;
                g.drawVerticalLine (juce::roundToInt (px), y0 + 1.0f, y0 + h - 1.0f);
            }
        }
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    std::vector<float> history;
    std::vector<bool> transientHistory;
    int writePos = 0;
    bool lastGateOpen = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GateDisplayComponent)
};
