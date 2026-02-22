#pragma once
#include <JuceHeader.h>


class BassLookAndFeel : public juce::LookAndFeel_V4
{
public:
    inline static const juce::Colour bg         { 0xff1a1a2e };
    inline static const juce::Colour surfaceDark { 0xff1d1d30 };
    inline static const juce::Colour surface    { 0xff252540 };
    inline static const juce::Colour border     { 0xff333355 };
    inline static const juce::Colour borderDim  { 0xff444466 };
    inline static const juce::Colour iconDim    { 0xff555577 };
    inline static const juce::Colour textDim    { 0xff888899 };
    inline static const juce::Colour text       { 0xffaaaacc };
    inline static const juce::Colour accent     { 0xfffff100 };

    void drawRotarySlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPos,
                           float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override
    {
        const float cx = x + width  * 0.5f;
        const float cy = y + height * 0.5f;
        const float radius = juce::jmin (width, height) * 0.5f - 4.0f;

        const float toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // Full-range track arc
        {
            juce::Path track;
            track.addArc (cx - radius, cy - radius,
                          radius * 2.0f, radius * 2.0f,
                          rotaryStartAngle, rotaryEndAngle, true);
            g.setColour (border);
            g.strokePath (track, juce::PathStrokeType (3.0f,
                juce::PathStrokeType::curved,
                juce::PathStrokeType::rounded));
        }

        // Value arc
        {
            juce::Path value;
            value.addArc (cx - radius, cy - radius,
                          radius * 2.0f, radius * 2.0f,
                          rotaryStartAngle, toAngle, true);
            g.setColour (accent);
            g.strokePath (value, juce::PathStrokeType (3.0f,
                juce::PathStrokeType::curved,
                juce::PathStrokeType::rounded));
        }

        // Knob body
        const float bodyRadius = radius * 0.65f;
        g.setColour (surface);
        g.fillEllipse (cx - bodyRadius, cy - bodyRadius, bodyRadius * 2.0f, bodyRadius * 2.0f);

        // Pointer dot
        const float dotRadius = 3.0f;
        const float dotR = radius * 0.6f;
        const float dotX = cx + dotR * std::sin (toAngle);
        const float dotY = cy - dotR * std::cos (toAngle);
        g.setColour (accent);
        g.fillEllipse (dotX - dotRadius, dotY - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
    }

    void drawButtonBackground (juce::Graphics& g,
                               juce::Button& button,
                               const juce::Colour& /*backgroundColour*/,
                               bool /*isMouseOverButton*/,
                               bool /*isButtonDown*/) override
    {
        const bool on = button.getToggleState();
        const auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);

        g.setColour (on ? border : surfaceDark);
        g.fillRoundedRectangle (bounds, 4.0f);

        g.setColour (on ? accent : borderDim);
        g.drawRoundedRectangle (bounds, 4.0f, 1.0f);
    }

    void drawButtonText (juce::Graphics& g,
                         juce::TextButton& button,
                         bool /*isMouseOverButton*/,
                         bool /*isButtonDown*/) override
    {
        const bool on = button.getToggleState();
        g.setColour (on ? accent : textDim);
        g.setFont (juce::Font (11.0f, juce::Font::bold));
        g.drawFittedText (button.getButtonText(),
                          button.getLocalBounds(),
                          juce::Justification::centred, 1);
    }
};
