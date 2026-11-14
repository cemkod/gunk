#pragma once
#include <JuceHeader.h>
#include "UIConstants.h"


// Look based on docs/ui-concepts/F_restructure_fix.png:
// navy/purple panels, purple section header bars, warm-yellow accent,
// thick arc knobs with a line pointer, pill power switches, filled tabs.
class BassLookAndFeel : public juce::LookAndFeel_V4
{
public:
    inline static const juce::Colour bg          { 0xff1b1930 };  // editor background
    inline static const juce::Colour headerBar   { 0xff4b3d82 };  // section title bar (purple)
    inline static const juce::Colour surfaceDark { 0xff14122a };  // display wells, combo boxes
    inline static const juce::Colour surface     { 0xff2a2748 };  // section body
    inline static const juce::Colour border      { 0xff3a3660 };
    inline static const juce::Colour borderDim   { 0xff4a4572 };
    inline static const juce::Colour iconDim     { 0xff655f92 };
    inline static const juce::Colour textDim     { 0xff9a94c4 };
    inline static const juce::Colour text        { 0xffdcd6f2 };
    inline static const juce::Colour accent      { 0xfffcd03c };  // warm yellow
    inline static const juce::Colour accentText  { 0xff1b1930 };  // text drawn on accent fills
    inline static const juce::Colour buttonFill  { 0xff2e2858 };  // unselected tab / plain button
    inline static const juce::Colour knobBody    { 0xff221f3e };

    // Button styles, set via setButtonStyle(). When unset, the style is inferred:
    // radio-group buttons are tabs, toggling buttons are outlined, the rest are plain.
    enum class ButtonStyle { plain, tab, outline };

    static void setButtonStyle (juce::Button& b, ButtonStyle s)
    {
        b.getProperties().set (buttonStyleProp, (int) s);
        b.repaint();
    }

    BassLookAndFeel()
    {
        setColour (juce::PopupMenu::backgroundColourId,            surfaceDark);
        setColour (juce::PopupMenu::textColourId,                  text);
        setColour (juce::PopupMenu::headerTextColourId,            textDim);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, accent);
        setColour (juce::PopupMenu::highlightedTextColourId,       accentText);
        setColour (juce::ComboBox::backgroundColourId,             surfaceDark);
        setColour (juce::ComboBox::textColourId,                   accent);
        setColour (juce::ComboBox::arrowColourId,                  accent);
        setColour (juce::ComboBox::outlineColourId,                border);
        setColour (juce::TextEditor::backgroundColourId,           surfaceDark);
        setColour (juce::TextEditor::textColourId,                 accent);
        setColour (juce::TextEditor::highlightColourId,            accent.withAlpha (0.3f));
        setColour (juce::TextEditor::outlineColourId,              juce::Colours::transparentBlack);
        setColour (juce::TextEditor::focusedOutlineColourId,       accent.withAlpha (0.6f));
        setColour (juce::CaretComponent::caretColourId,            accent);
        setColour (juce::Label::textColourId,                      text);
        setColour (juce::AlertWindow::backgroundColourId,          surface);
        setColour (juce::AlertWindow::textColourId,                text);
        setColour (juce::AlertWindow::outlineColourId,             border);
    }

    static void setupRotarySlider (juce::Slider& s, juce::Label& l,
                                    const juce::String& name, juce::Component& parent)
    {
        s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, UIConst::sliderTextBoxW, UIConst::sliderTextBoxH);
        s.setColour (juce::Slider::textBoxTextColourId,       accent);
        s.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        s.setColour (juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
        s.setColour (juce::Slider::textBoxHighlightColourId,  accent.withAlpha (0.3f));
        parent.addAndMakeVisible (s);

        l.setText (name, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.setFont (juce::Font (UIConst::fontKnobLabel, juce::Font::bold));
        l.setColour (juce::Label::textColourId, accent);
        parent.addAndMakeVisible (l);
    }

    static void setupLinearSlider (juce::Slider& s, juce::Label& l,
                                    const juce::String& name, juce::Component& parent)
    {
        s.setSliderStyle (juce::Slider::LinearHorizontal);
        s.setTextBoxStyle (juce::Slider::TextBoxRight, false, 42, 14);
        s.setColour (juce::Slider::textBoxTextColourId,       accent);
        s.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        s.setColour (juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
        s.setColour (juce::Slider::textBoxHighlightColourId,  accent.withAlpha (0.3f));
        parent.addAndMakeVisible (s);

        l.setText (name, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centredLeft);
        l.setFont (juce::Font (UIConst::fontKnobLabel, juce::Font::bold));
        l.setColour (juce::Label::textColourId, accent);
        parent.addAndMakeVisible (l);
    }

    //==========================================================================
    // Knob: thick yellow value arc over a dark track, dark body, line pointer.
    void drawRotarySlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPos,
                           float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override
    {
        const float stroke = UIConst::knobArcStrokeWidth;
        const float cx = x + width  * 0.5f;
        const float cy = y + height * 0.5f;
        const float radius = juce::jmin (width, height) * 0.5f - stroke * 0.5f - 1.0f;
        if (radius <= 2.0f) return;

        const float toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        const bool enabled = slider.isEnabled();
        const auto valueCol = enabled ? accent : iconDim;

        // Bipolar sliders fill from the centre of the range
        const bool isBipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;
        float fromAngle = rotaryStartAngle;
        if (isBipolar)
        {
            const auto zeroPos = (float) slider.valueToProportionOfLength (0.0);
            fromAngle = rotaryStartAngle + zeroPos * (rotaryEndAngle - rotaryStartAngle);
        }

        const juce::PathStrokeType arcStroke (stroke, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded);

        // Track
        rotaryTrack.clear();
        rotaryTrack.addCentredArc (cx, cy, radius, radius, 0.0f,
                                   rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (surfaceDark);
        g.strokePath (rotaryTrack, arcStroke);

        // Value arc
        if (std::abs (toAngle - fromAngle) > 0.001f)
        {
            rotaryValue.clear();
            rotaryValue.addCentredArc (cx, cy, radius, radius, 0.0f,
                                       juce::jmin (fromAngle, toAngle),
                                       juce::jmax (fromAngle, toAngle), true);
            g.setColour (valueCol);
            g.strokePath (rotaryValue, arcStroke);
        }

        // Body: dark disc with a subtle rim
        const float bodyR = radius * UIConst::knobBodyRadiusRatio;
        g.setColour (knobBody);
        g.fillEllipse (cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f);
        g.setColour (border);
        g.drawEllipse (cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.0f);

        // Pointer line from inside the body out towards the rim
        const float sinA = std::sin (toAngle), cosA = -std::cos (toAngle);
        const float r0 = bodyR * 0.35f;
        const float r1 = bodyR * UIConst::knobPointerPosRatio;
        g.setColour (valueCol);
        g.drawLine (cx + r0 * sinA, cy + r0 * cosA,
                    cx + r1 * sinA, cy + r1 * cosA,
                    UIConst::knobPointerWidth);
    }

    //==========================================================================
    // Buttons
    void drawButtonBackground (juce::Graphics& g,
                               juce::Button& button,
                               const juce::Colour& /*backgroundColour*/,
                               bool isMouseOverButton,
                               bool isButtonDown) override
    {
        const bool on = button.getToggleState();
        const auto b  = button.getLocalBounds().toFloat().reduced (0.5f);
        const float r = UIConst::buttonCornerRadius;
        const float hover = button.isEnabled() ? (isButtonDown ? 0.18f : (isMouseOverButton ? 0.08f : 0.0f)) : 0.0f;

        switch (getButtonStyle (button))
        {
            case ButtonStyle::tab:
                g.setColour (on ? accent : buttonFill.brighter (hover));
                g.fillRoundedRectangle (b, r);
                break;

            case ButtonStyle::outline:
                if (on)
                {
                    g.setColour (accent.brighter (hover));
                    g.fillRoundedRectangle (b, r);
                }
                else
                {
                    g.setColour (accent.withAlpha (hover));
                    g.fillRoundedRectangle (b, r);
                    g.setColour (button.isEnabled() ? accent : iconDim);
                    g.drawRoundedRectangle (b.reduced (0.5f), r, 1.5f);
                }
                break;

            case ButtonStyle::plain:
                g.setColour (on ? accent : buttonFill.brighter (hover));
                g.fillRoundedRectangle (b, r);
                break;
        }
    }

    void drawButtonText (juce::Graphics& g,
                         juce::TextButton& button,
                         bool /*isMouseOverButton*/,
                         bool /*isButtonDown*/) override
    {
        const bool on = button.getToggleState();
        juce::Colour c;

        if (! button.isEnabled())
            c = iconDim;
        else if (on)
            c = accentText;
        else
            c = getButtonStyle (button) == ButtonStyle::outline ? accent : text;

        g.setColour (c);
        g.setFont (juce::Font (UIConst::fontButtonText, juce::Font::bold));
        g.drawFittedText (button.getButtonText(),
                          button.getLocalBounds().reduced (3, 0),
                          juce::Justification::centred, 1);
    }

    // Checkbox-style ToggleButtons: small rounded yellow box with a tick
    void drawTickBox (juce::Graphics& g, juce::Component& c,
                      float x, float y, float w, float h,
                      bool ticked, bool isEnabled, bool, bool) override
    {
        const juce::Rectangle<float> box (x, y, w, h);
        const auto col = isEnabled ? accent : iconDim;
        if (ticked)
        {
            g.setColour (col);
            g.fillRoundedRectangle (box, 2.0f);
            juce::Path tick;
            tick.startNewSubPath (box.getX() + w * 0.22f, box.getCentreY());
            tick.lineTo (box.getX() + w * 0.42f, box.getBottom() - h * 0.25f);
            tick.lineTo (box.getRight() - w * 0.2f, box.getY() + h * 0.25f);
            g.setColour (accentText);
            g.strokePath (tick, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
        }
        else
        {
            g.setColour (surfaceDark);
            g.fillRoundedRectangle (box, 2.0f);
            g.setColour (col);
            g.drawRoundedRectangle (box.reduced (0.5f), 2.0f, 1.2f);
        }
        juce::ignoreUnused (c);
    }

    //==========================================================================
    // Linear slider: thick dark track, yellow fill, rounded yellow thumb
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
        const auto valueCol = slider.isEnabled() ? accent : iconDim;

        const float trackH = UIConst::linearTrackH;
        const float trackY = y + (height - trackH) * 0.5f;
        const float trackX = (float) x;
        const float trackW = (float) width;

        g.setColour (surfaceDark);
        g.fillRoundedRectangle (trackX, trackY, trackW, trackH, trackH * 0.5f);

        const float thumbX = sliderPos;
        float fillL, fillR;

        if (isBipolar)
        {
            const float zeroFrac = (float) ((-slider.getMinimum()) / (slider.getMaximum() - slider.getMinimum()));
            const float zeroX    = trackX + zeroFrac * trackW;
            fillL = juce::jmin (zeroX, thumbX);
            fillR = juce::jmax (zeroX, thumbX);

            g.setColour (textDim);
            g.fillRect (zeroX - 0.5f, trackY - 2.0f, 1.0f, trackH + 4.0f);
        }
        else
        {
            fillL = trackX;
            fillR = thumbX;
        }

        if (fillR > fillL + 0.5f)
        {
            g.setColour (valueCol);
            g.fillRoundedRectangle (fillL, trackY, fillR - fillL, trackH, trackH * 0.5f);
        }

        // Thumb: rounded pill, slightly taller than the track, with a dark rim
        const float thumbW = UIConst::linearThumbW;
        const float thumbH = juce::jmin ((float) height * 0.85f, trackH * 2.6f);
        const juce::Rectangle<float> thumb (thumbX - thumbW * 0.5f, y + (height - thumbH) * 0.5f,
                                            thumbW, thumbH);
        g.setColour (valueCol);
        g.fillRoundedRectangle (thumb, thumbW * 0.5f);
        g.setColour (surface);
        g.drawRoundedRectangle (thumb, thumbW * 0.5f, 1.0f);
    }

    //==========================================================================
    // Combo box: dark well, yellow text, yellow triangle
    void drawComboBox (juce::Graphics& g, int w, int h, bool /*isButtonDown*/,
                       int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
                       juce::ComboBox& box) override
    {
        const juce::Rectangle<float> b (0.0f, 0.0f, (float) w, (float) h);
        g.setColour (surfaceDark);
        g.fillRoundedRectangle (b, UIConst::comboBoxCornerRadius);

        const float aw = juce::jmin (9.0f, h * 0.45f);
        const float ah = aw * 0.6f;
        const float ax = (float) w - aw - 6.0f;
        const float ay = (float) h * 0.5f - ah * 0.5f;
        comboArrow.clear();
        comboArrow.addTriangle (ax, ay, ax + aw, ay, ax + aw * 0.5f, ay + ah);
        g.setColour (box.isEnabled() ? accent : iconDim);
        g.fillPath (comboArrow);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override
    {
        return juce::Font (UIConst::uiFontSize, juce::Font::bold);
    }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (4, 0, box.getWidth() - 20, box.getHeight());
        label.setFont (getComboBoxFont (box));
        label.setColour (juce::Label::textColourId, box.isEnabled() ? accent : iconDim);
    }

    juce::Font getPopupMenuFont() override
    {
        return juce::Font (UIConst::uiFontSize + 2.0f);
    }

    //==========================================================================
    // Slider value text box: no box, value text centred
    juce::Label* createSliderTextBox (juce::Slider& slider) override
    {
        auto* l = juce::LookAndFeel_V4::createSliderTextBox (slider);
        l->setFont (juce::Font (UIConst::fontKnobValue));
        return l;
    }

private:
    inline static const juce::Identifier buttonStyleProp { "jqButtonStyle" };

    static ButtonStyle getButtonStyle (const juce::Button& b)
    {
        if (const auto* v = b.getProperties().getVarPointer (buttonStyleProp))
            return (ButtonStyle) (int) *v;
        if (b.getRadioGroupId() != 0)        return ButtonStyle::tab;
        if (b.getClickingTogglesState())     return ButtonStyle::outline;
        return ButtonStyle::plain;
    }

    juce::Path rotaryTrack, rotaryValue, comboArrow;
};
