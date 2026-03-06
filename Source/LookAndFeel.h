#pragma once
#include <JuceHeader.h>
#include "UIConstants.h"


class BassLookAndFeel : public juce::LookAndFeel_V4
{
public:
    inline static const juce::Colour bg         { 0xff1a1a2e };
    inline static const juce::Colour headerBar  { 0xff141426 };
    inline static const juce::Colour surfaceDark { 0xff1d1d30 };
    inline static const juce::Colour surface    { 0xff252540 };
    inline static const juce::Colour border     { 0xff333355 };
    inline static const juce::Colour borderDim  { 0xff444466 };
    inline static const juce::Colour iconDim    { 0xff555577 };
    inline static const juce::Colour textDim    { 0xff888899 };
    inline static const juce::Colour text       { 0xffaaaacc };
    inline static const juce::Colour accent     { 0xfffff100 };

    static void setupRotarySlider (juce::Slider& s, juce::Label& l,
                                    const juce::String& name, juce::Component& parent)
    {
        s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, UIConst::sliderTextBoxW, UIConst::sliderTextBoxH);
        s.setColour (juce::Slider::textBoxTextColourId,       text);
        s.setColour (juce::Slider::textBoxBackgroundColourId, surfaceDark);
        s.setColour (juce::Slider::textBoxOutlineColourId,    borderDim);
        parent.addAndMakeVisible (s);

        l.setText (name, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.setFont (juce::Font (UIConst::fontKnobLabel, juce::Font::bold));
        l.setColour (juce::Label::textColourId, text);
        parent.addAndMakeVisible (l);
    }

    static void setupLinearSlider (juce::Slider& s, juce::Label& l,
                                    const juce::String& name, juce::Component& parent)
    {
        s.setSliderStyle (juce::Slider::LinearHorizontal);
        s.setTextBoxStyle (juce::Slider::TextBoxRight, false, 42, 14);
        s.setColour (juce::Slider::textBoxTextColourId,       text);
        s.setColour (juce::Slider::textBoxBackgroundColourId, surfaceDark);
        s.setColour (juce::Slider::textBoxOutlineColourId,    borderDim);
        parent.addAndMakeVisible (s);

        l.setText (name, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centredLeft);
        l.setFont (juce::Font (UIConst::fontKnobLabel, juce::Font::bold));
        l.setColour (juce::Label::textColourId, text);
        parent.addAndMakeVisible (l);
    }

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
            g.strokePath (track, juce::PathStrokeType (UIConst::knobArcStrokeWidth,
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
            g.strokePath (value, juce::PathStrokeType (UIConst::knobArcStrokeWidth,
                juce::PathStrokeType::curved,
                juce::PathStrokeType::rounded));
        }

        // Knob body
        const float bodyRadius = radius * UIConst::knobBodyRadiusRatio;
        g.setColour (surface);
        g.fillEllipse (cx - bodyRadius, cy - bodyRadius, bodyRadius * 2.0f, bodyRadius * 2.0f);

        // Pointer dot
        const float dotRadius = UIConst::knobPointerRadius;
        const float dotR = radius * UIConst::knobPointerPosRatio;
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
        g.fillRoundedRectangle (bounds, UIConst::buttonCornerRadius);

        g.setColour (on ? accent : borderDim);
        g.drawRoundedRectangle (bounds, UIConst::buttonCornerRadius, 1.0f);
    }

    void drawButtonText (juce::Graphics& g,
                         juce::TextButton& button,
                         bool /*isMouseOverButton*/,
                         bool /*isButtonDown*/) override
    {
        const bool on = button.getToggleState();
        g.setColour (on ? accent : textDim);
        g.setFont (juce::Font (UIConst::fontButtonText, juce::Font::bold));
        g.drawFittedText (button.getButtonText(),
                          button.getLocalBounds(),
                          juce::Justification::centred, 1);
    }

    void drawLinearSlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle style,
                           juce::Slider& slider) override
    {
        if (style != juce::Slider::LinearHorizontal)
        {
            juce::LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos,
                                                    minSliderPos, maxSliderPos, style, slider);
            return;
        }

        const bool isBipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;

        const float trackH = 4.0f;
        const float trackY = y + (height - trackH) * 0.5f;
        const float trackX = (float) x;
        const float trackW = (float) width;

        // Track background
        g.setColour (border);
        g.fillRoundedRectangle (trackX, trackY, trackW, trackH, trackH * 0.5f);

        const float thumbX = sliderPos;
        float fillL, fillR;

        if (isBipolar)
        {
            const float zeroFrac = (float) ((-slider.getMinimum()) / (slider.getMaximum() - slider.getMinimum()));
            const float zeroX    = trackX + zeroFrac * trackW;
            fillL = juce::jmin (zeroX, thumbX);
            fillR = juce::jmax (zeroX, thumbX);

            // Center tick
            g.setColour (textDim);
            g.fillRect (zeroX - 0.5f, trackY - 2.0f, 1.0f, trackH + 4.0f);
        }
        else
        {
            fillL = trackX;
            fillR = thumbX;
        }

        // Value fill
        if (fillR > fillL + 0.5f)
        {
            g.setColour (accent);
            g.fillRoundedRectangle (fillL, trackY, fillR - fillL, trackH, trackH * 0.5f);
        }

        // Thumb
        const float thumbW = 8.0f;
        const float thumbH = (float) height * 0.7f;
        g.setColour (accent);
        g.fillRoundedRectangle (thumbX - thumbW * 0.5f, y + (height - thumbH) * 0.5f,
                                thumbW, thumbH, 2.0f);
    }

    void drawComboBox (juce::Graphics& g, int w, int h, bool /*isButtonDown*/,
                       int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
                       juce::ComboBox&) override
    {
        g.setColour (surface);
        g.fillRoundedRectangle (0.0f, 0.0f, (float) w, (float) h, UIConst::comboBoxCornerRadius);
        g.setColour (border);
        g.drawRoundedRectangle (0.5f, 0.5f, (float) w - 1.0f, (float) h - 1.0f, UIConst::comboBoxCornerRadius, 1.0f);

        // Draw arrow
        const float arrowX = (float) w - 14.0f;
        const float arrowY = (float) h * 0.5f;
        juce::Path arrow;
        arrow.addTriangle (arrowX, arrowY - 3.0f,
                           arrowX + 8.0f, arrowY - 3.0f,
                           arrowX + 4.0f, arrowY + 3.0f);
        g.setColour (textDim);
        g.fillPath (arrow);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override
    {
        return juce::Font (UIConst::uiFontSize, juce::Font::bold);
    }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (4, 0, box.getWidth() - 20, box.getHeight());
        label.setFont (getComboBoxFont (box));
        label.setColour (juce::Label::textColourId, text);
    }
};
