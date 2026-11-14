#pragma once
#include <JuceHeader.h>
#include <BinaryData.h>
#include "UIConstants.h"


// Look based on docs/ui-concepts/G2_hierarchy.png and G2_modmatrix.png:
// navy/purple panels with a subtle header strip, warm-yellow accent for live values,
// thick arc knobs in three sizes, pill power switches, segmented controls.
class BassLookAndFeel : public juce::LookAndFeel_V4
{
public:
    inline static const juce::Colour bg          { 0xff1b1930 };  // editor background
    inline static const juce::Colour headerBar   { 0xff221f3e };  // section title strip
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
    inline static const juce::Colour knobBody    { 0xff35306a };

    // Mod source colours, indexed by the modSlot*Source choice index
    // (None, Input Env, Pitch, ADSR, LFO). Display names live next to them so they can't drift.
    inline static const std::array<juce::Colour, 5> modSourceColours {
        juce::Colour (0xff9a94c4),   // None
        juce::Colour (0xffff8a3d),   // Input Env (orange)
        juce::Colour (0xff4cd964),   // Pitch (green)
        juce::Colour (0xff3ec9f0),   // ADSR (cyan)
        juce::Colour (0xffff4fb3)    // LFO (magenta)
    };
    inline static const juce::StringArray modSourceNames { "None", "Input Env", "Pitch", "ADSR", "LFO" };

    static juce::Colour modSourceColour (int sourceIndex)
    {
        return modSourceColours[(size_t) juce::jlimit (0, (int) modSourceColours.size() - 1, sourceIndex)];
    }

    // Name labels are quiet; the value under them is accent, so yellow means "a live value".
    inline static const juce::Colour knobName = text.withAlpha (0.8f);

    // Knob size tiers, set via placeKnob() and stored as a component property.
    enum class KnobSize { small, standard, hero };

    static int knobDiameter (KnobSize k)
    {
        switch (k)
        {
            case KnobSize::small: return UIConst::knobSmallD;
            case KnobSize::hero:  return UIConst::knobHeroD;
            default:              return UIConst::knobStandardD;
        }
    }

    static int knobCellHeight (KnobSize k)
    {
        return knobDiameter (k) + UIConst::knobNameGap + UIConst::knobNameH + UIConst::knobValueH;
    }

    static KnobSize getKnobSize (const juce::Component& c)
    {
        if (const auto* v = c.getProperties().getVarPointer (knobSizeProp))
            return (KnobSize) (int) *v;
        return KnobSize::standard;
    }

    // A coloured dot before a combo box's text (mod source combos). Transparent removes it.
    static void setComboDotColour (juce::ComboBox& c, juce::Colour col)
    {
        const auto argb = (juce::int64) col.getARGB();
        if (c.getProperties()[comboDotProp] == juce::var (argb)) return;
        c.getProperties().set (comboDotProp, argb);
        c.resized();      // re-runs positionComboBoxText
        c.repaint();
    }

    static juce::Colour getComboDotColour (const juce::ComboBox& c)
    {
        if (const auto* v = c.getProperties().getVarPointer (comboDotProp))
            return juce::Colour ((juce::uint32) (juce::int64) *v);
        return juce::Colours::transparentBlack;
    }

    // Thick track with a round thumb (the mod amount bars)
    static void setBarStyle (juce::Slider& s) { s.getProperties().set (barStyleProp, true); }

    // Larger text for a particular combo box (e.g. the preset name).
    static void setComboFontHeight (juce::ComboBox& c, float uiFontHeight)
    {
        c.getProperties().set (comboFontProp, uiFontHeight);
        c.lookAndFeelChanged();
    }

    // Fill colour for a linear slider (mod amount bars use the source colour).
    static void setFillColour (juce::Slider& s, juce::Colour c)
    {
        s.getProperties().set (fillColourProp, (juce::int64) c.getARGB());
        s.repaint();
    }

    // Button styles, set via setButtonStyle(). When unset, the style is inferred:
    // radio-group buttons are tabs, toggling buttons are outlined, the rest are plain.
    enum class ButtonStyle { plain, tab, outline };

    static void setButtonStyle (juce::Button& b, ButtonStyle s)
    {
        b.getProperties().set (buttonStyleProp, (int) s);
        b.repaint();
    }

    // UI font: Barlow Semi Condensed (SIL OFL, Resources/Fonts), embedded as binary data.
    // JUCE 7 resolves typefaces through the *default* LookAndFeel, so fonts are built
    // explicitly from the embedded typefaces rather than via getTypefaceForFont().
    static juce::Font uiFont (float height, bool bold = false)
    {
        static const juce::Typeface::Ptr regular = juce::Typeface::createSystemTypefaceFor (
            BinaryData::BarlowSemiCondensedMedium_ttf, BinaryData::BarlowSemiCondensedMedium_ttfSize);
        static const juce::Typeface::Ptr semiBold = juce::Typeface::createSystemTypefaceFor (
            BinaryData::BarlowSemiCondensedSemiBold_ttf, BinaryData::BarlowSemiCondensedSemiBold_ttfSize);
        return juce::Font (bold ? semiBold : regular).withHeight (height * UIConst::fontScale);
    }

    // Swap a default-sans font for the UI font, keeping its size and weight.
    static juce::Font toUiFont (const juce::Font& f)
    {
        if (f.getTypefaceName() == juce::Font::getDefaultSansSerifFontName())
            return uiFont (f.getHeight(), f.isBold());
        return f;
    }

    // Knob cell (top to bottom): dial, name label, value text box. The cell's height
    // comes from the tier, centred vertically in `cell`; the width is the whole cell.
    // The name label overlays the slider and ignores the mouse, so dragging it turns the knob.
    static void placeKnob (juce::Slider& s, juce::Label& l, juce::Rectangle<int> cell,
                           KnobSize size = KnobSize::standard)
    {
        if (getKnobSize (s) != size)
        {
            s.getProperties().set (knobSizeProp, (int) size);
            s.lookAndFeelChanged();   // rebuild the value box so it picks up the tier's font
        }
        cell = cell.withSizeKeepingCentre (cell.getWidth(), juce::jmin (cell.getHeight(), knobCellHeight (size)));
        s.setBounds (cell);
        cell.removeFromBottom (UIConst::knobValueH);
        l.setBounds (cell.removeFromBottom (UIConst::knobNameH));
    }

    // Lay out a row of knob cells of equal width across `row`.
    static void placeKnobRow (juce::Rectangle<int> row,
                              std::initializer_list<std::pair<juce::Slider*, juce::Label*>> knobs,
                              int columns = 0, KnobSize size = KnobSize::standard)
    {
        const int n = columns > 0 ? columns : (int) knobs.size();
        const int w = row.getWidth() / n;
        int i = 0;
        for (auto [s, l] : knobs)
            placeKnob (*s, *l, ++i == n ? row : row.removeFromLeft (w), size);
    }

    // A small dim caption, e.g. "WINDOW" or "SHAPE" next to a combo.
    static void setupCaption (juce::Label& l, const juce::String& textToShow, juce::Component& parent,
                              juce::Justification j = juce::Justification::centredLeft)
    {
        l.setText (textToShow, juce::dontSendNotification);
        l.setFont (uiFont (UIConst::fontCaption, true));
        l.setColour (juce::Label::textColourId, knobName);
        l.setJustificationType (j);
        l.setBorderSize ({});
        l.setInterceptsMouseClicks (false, false);
        parent.addAndMakeVisible (l);
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
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, UIConst::knobValueH);
        s.getProperties().set (knobNameProp, name);
        s.setWantsKeyboardFocus (true);   // click a knob, then the arrow keys nudge it
        s.setColour (juce::Slider::textBoxTextColourId,       accent);
        s.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        s.setColour (juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
        s.setColour (juce::Slider::textBoxHighlightColourId,  accent.withAlpha (0.3f));
        parent.addAndMakeVisible (s);

        l.setText (name, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.setFont (uiFont (UIConst::fontKnobLabel, true));
        l.setColour (juce::Label::textColourId, knobName);
        l.setInterceptsMouseClicks (false, false);
        l.setBorderSize ({});
        parent.addAndMakeVisible (l);
    }

    static void setupLinearSlider (juce::Slider& s, juce::Label& l,
                                    const juce::String& name, juce::Component& parent)
    {
        s.setSliderStyle (juce::Slider::LinearHorizontal);
        s.setTextBoxStyle (juce::Slider::TextBoxRight, false, 56, 20);
        s.setColour (juce::Slider::textBoxTextColourId,       accent);
        s.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        s.setColour (juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
        s.setColour (juce::Slider::textBoxHighlightColourId,  accent.withAlpha (0.3f));
        parent.addAndMakeVisible (s);

        l.setText (name, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centredLeft);
        l.setFont (uiFont (UIConst::fontKnobLabel, true));
        l.setColour (juce::Label::textColourId, knobName);
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
        const float cx = x + width  * 0.5f;
        const float cy = y + height * 0.5f;
        const float outerR = juce::jmin (width, height) * 0.5f - 1.0f;
        const float stroke = juce::jmax (2.5f, outerR * UIConst::knobArcStrokeRatio);
        const float radius = outerR - stroke * 0.5f;   // arc centreline
        if (radius <= 4.0f) return;

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

        // Body: dark disc inset from the arc, lit slightly from the top, with a thin rim
        const float bodyR = radius - stroke * 0.5f - juce::jmax (1.5f, outerR * UIConst::knobBodyGapRatio);
        const juce::Rectangle<float> body (cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f);
        g.setGradientFill (juce::ColourGradient (knobBody.brighter (0.15f), cx, body.getY(),
                                                 knobBody.darker (0.35f),   cx, body.getBottom(), false));
        g.fillEllipse (body);
        g.setColour (surfaceDark);
        g.drawEllipse (body.reduced (0.6f), 1.2f);

        // Pointer line from inside the body out to its rim
        const float sinA = std::sin (toAngle), cosA = -std::cos (toAngle);
        const float r0 = bodyR * UIConst::knobPointerInnerRatio;
        const float r1 = bodyR * UIConst::knobPointerOuterRatio;
        knobPointer.clear();
        knobPointer.startNewSubPath (cx + r0 * sinA, cy + r0 * cosA);
        knobPointer.lineTo          (cx + r1 * sinA, cy + r1 * cosA);
        g.setColour (valueCol);
        g.strokePath (knobPointer, juce::PathStrokeType (juce::jmax (1.8f, outerR * UIConst::knobPointerWidthRatio),
                                                         juce::PathStrokeType::curved,
                                                         juce::PathStrokeType::rounded));

        // Mod sources driving this knob: small dots in the arc's gap at the bottom
        if (const auto* dots = slider.getProperties()[modDotsProp].getArray(); dots != nullptr && ! dots->isEmpty())
        {
            const float d = juce::jmax (4.0f, outerR * 0.13f), gap = d * 0.6f;
            const int n = dots->size();
            float dotX = cx - ((float) n * d + (float) (n - 1) * gap) * 0.5f;
            const float dotY = cy + outerR - d;
            for (const auto& v : *dots)
            {
                g.setColour (juce::Colour ((juce::uint32) (juce::int64) v));
                g.fillEllipse (dotX, dotY, d, d);
                dotX += d + gap;
            }
        }

        // Keyboard focus: a faint ring (arrow keys then nudge the value)
        if (slider.hasKeyboardFocus (false))
        {
            g.setColour (accent.withAlpha (0.35f));
            g.drawEllipse (juce::Rectangle<float> (outerR * 2.0f, outerR * 2.0f).withCentre ({ cx, cy }).expanded (1.0f), 1.0f);
        }
    }

    // Mod indicator dots under a knob, one per modulating slot, in the source colours.
    static void setModDots (juce::Slider& s, const juce::Array<juce::Colour>& colours)
    {
        juce::Array<juce::var> arr;
        for (auto c : colours) arr.add ((juce::int64) c.getARGB());
        const juce::var v (arr);
        if (s.getProperties()[modDotsProp] == v) return;
        s.getProperties().set (modDotsProp, v);
        s.repaint();
    }

    // The ModTarget a knob controls (0 = none); used for tooltips and mod dots.
    static void setModTarget (juce::Slider& s, int modTarget) { s.getProperties().set (modTargetProp, modTarget); }
    static int  getModTarget (const juce::Slider& s)          { return (int) s.getProperties()[modTargetProp]; }
    static juce::String getKnobName (const juce::Slider& s)   { return s.getProperties()[knobNameProp].toString(); }

    // Knob cells: square knob on top, then the name label (placed by placeKnob), then the value
    juce::Slider::SliderLayout getSliderLayout (juce::Slider& s) override
    {
        if (! s.isRotary() || s.getTextBoxPosition() != juce::Slider::TextBoxBelow)
            return juce::LookAndFeel_V4::getSliderLayout (s);

        auto b = s.getLocalBounds();
        juce::Slider::SliderLayout layout;
        layout.textBoxBounds = b.removeFromBottom (UIConst::knobValueH);
        b.removeFromBottom (UIConst::knobNameH + UIConst::knobNameGap);
        const int d = juce::jmin (b.getWidth(), b.getHeight(), knobDiameter (getKnobSize (s)));
        layout.sliderBounds = b.withSizeKeepingCentre (d, d);
        return layout;
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
                // On: accent outline over a faint accent tint. Off: quiet border.
                g.setColour (accent.withAlpha ((on ? 0.16f : 0.0f) + hover));
                g.fillRoundedRectangle (b, r);
                g.setColour (! button.isEnabled() ? iconDim : (on ? accent : borderDim));
                g.drawRoundedRectangle (b.reduced (0.5f), r, 1.5f);
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

        const bool outline = getButtonStyle (button) == ButtonStyle::outline;
        if (! button.isEnabled())
            c = iconDim;
        else if (outline)
            c = on ? accent : textDim;
        else
            c = on ? accentText : text;

        g.setColour (c);
        g.setFont (uiFont (UIConst::fontButtonText, true));
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
        auto fill = accent;
        if (const auto* v = slider.getProperties().getVarPointer (fillColourProp))
            fill = juce::Colour ((juce::uint32) (juce::int64) *v);
        const auto valueCol = slider.isEnabled() ? fill : iconDim;

        // Mod amount bars: a thicker track and a round thumb
        const bool bar = (bool) slider.getProperties()[barStyleProp];
        const float trackH = bar ? 10.0f : UIConst::linearTrackH;
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

        if (bar)
        {
            const float d = trackH + 8.0f;
            const auto knob = juce::Rectangle<float> (d, d).withCentre ({ thumbX, trackY + trackH * 0.5f });
            g.setColour (valueCol);
            g.fillEllipse (knob);
            g.setColour (surface);
            g.drawEllipse (knob, 1.5f);
            return;
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

        g.setColour (box.findColour (juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle (b.reduced (0.5f), UIConst::comboBoxCornerRadius, 1.0f);

        if (const auto dot = getComboDotColour (box); ! dot.isTransparent())
        {
            g.setColour (dot);
            g.fillEllipse (juce::Rectangle<float> (10.0f, (float) h * 0.5f - 6.0f, 12.0f, 12.0f));
        }

        const float aw = juce::jmin (11.0f, h * 0.4f);
        const float ah = aw * 0.6f;
        const float ax = (float) w - aw - 9.0f;
        const float ay = (float) h * 0.5f - ah * 0.5f;
        comboArrow.clear();
        comboArrow.addTriangle (ax, ay, ax + aw, ay, ax + aw * 0.5f, ay + ah);
        g.setColour (box.isEnabled() ? accent : iconDim);
        g.fillPath (comboArrow);
    }

    juce::Font getComboBoxFont (juce::ComboBox& box) override
    {
        if (const auto* v = box.getProperties().getVarPointer (comboFontProp))
            return uiFont ((float) *v, true);
        return uiFont (UIConst::uiFontSize, true);
    }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        const int x = getComboDotColour (box).isTransparent() ? 6 : 28;
        label.setBounds (x, 0, box.getWidth() - x - 18, box.getHeight());
        label.setFont (getComboBoxFont (box));
        label.setColour (juce::Label::textColourId, box.isEnabled() ? accent : iconDim);
    }

    juce::Font getPopupMenuFont() override
    {
        return uiFont (UIConst::px (17.0f));
    }

    // Anything else that asks the LookAndFeel for a font gets the UI font
    juce::Font getLabelFont (juce::Label& l) override
    {
        // A rotary slider's value box: size from the knob tier
        if (auto* s = dynamic_cast<juce::Slider*> (l.getParentComponent()); s != nullptr && s->isRotary())
        {
            switch (getKnobSize (*s))
            {
                case KnobSize::small: return uiFont (UIConst::fontKnobValueSmall, true);
                case KnobSize::hero:  return uiFont (UIConst::fontKnobValueHero, true);
                default:              return uiFont (UIConst::fontKnobValue, true);
            }
        }
        return toUiFont (l.getFont());
    }
    juce::Font getTextButtonFont (juce::TextButton&, int h) override { return uiFont (juce::jmin (UIConst::fontButtonText, h * 0.6f), true); }
    juce::Font getSliderPopupFont (juce::Slider&) override          { return uiFont (UIConst::fontKnobValue); }
    juce::Font getAlertWindowTitleFont() override                   { return uiFont (16.0f, true); }
    juce::Font getAlertWindowMessageFont() override                 { return uiFont (14.0f); }
    juce::Font getAlertWindowFont() override                        { return uiFont (13.0f); }

    //==========================================================================
    // Slider value text box: no box, value text centred
    juce::Label* createSliderTextBox (juce::Slider& slider) override
    {
        auto* l = juce::LookAndFeel_V4::createSliderTextBox (slider);
        l->setFont (getLabelFont (*l));   // not parented yet; getLabelFont re-resolves the tier at paint
        if (slider.isRotary())
            l->setBorderSize ({});
        return l;
    }

    //==========================================================================
    // Tooltips: dark panel, first line (the name) in accent, the rest in text colour
    static juce::TextLayout layoutTooltip (const juce::String& tip)
    {
        juce::AttributedString s;
        s.setJustification (juce::Justification::topLeft);
        const auto firstLine = tip.upToFirstOccurrenceOf ("\n", false, false);
        const auto rest      = tip.fromFirstOccurrenceOf ("\n", true, false);
        s.append (firstLine, uiFont (UIConst::px (15.0f), true), accent);
        if (rest.isNotEmpty())
            s.append (rest, uiFont (UIConst::px (14.0f)), text);
        juce::TextLayout tl;
        tl.createLayoutWithBalancedLineLengths (s, 360.0f);
        return tl;
    }

    juce::Rectangle<int> getTooltipBounds (const juce::String& tipText, juce::Point<int> screenPos,
                                           juce::Rectangle<int> parentArea) override
    {
        const auto tl = layoutTooltip (tipText);
        const int w = (int) std::ceil (tl.getWidth()) + 20, h = (int) std::ceil (tl.getHeight()) + 14;
        return juce::Rectangle<int> (screenPos.x > parentArea.getCentreX() ? screenPos.x - (w + 12) : screenPos.x + 24,
                                     screenPos.y > parentArea.getCentreY() ? screenPos.y - (h + 6)  : screenPos.y + 6,
                                     w, h).constrainedWithin (parentArea);
    }

    void drawTooltip (juce::Graphics& g, const juce::String& tipText, int width, int height) override
    {
        const auto b = juce::Rectangle<float> ((float) width, (float) height);
        g.setColour (surfaceDark);
        g.fillRoundedRectangle (b, 4.0f);
        g.setColour (borderDim);
        g.drawRoundedRectangle (b.reduced (0.5f), 4.0f, 1.0f);
        layoutTooltip (tipText).draw (g, b.reduced (10.0f, 7.0f));
    }

private:
    inline static const juce::Identifier buttonStyleProp { "jqButtonStyle" };
    inline static const juce::Identifier knobSizeProp    { "jqKnobSize" };
    inline static const juce::Identifier fillColourProp  { "jqFillColour" };
    inline static const juce::Identifier comboFontProp   { "jqComboFont" };
    inline static const juce::Identifier comboDotProp    { "jqComboDot" };
    inline static const juce::Identifier barStyleProp    { "jqBarStyle" };
    inline static const juce::Identifier modDotsProp     { "jqModDots" };
    inline static const juce::Identifier modTargetProp   { "jqModTarget" };
    inline static const juce::Identifier knobNameProp    { "jqKnobName" };

    static ButtonStyle getButtonStyle (const juce::Button& b)
    {
        if (const auto* v = b.getProperties().getVarPointer (buttonStyleProp))
            return (ButtonStyle) (int) *v;
        if (b.getRadioGroupId() != 0)        return ButtonStyle::tab;
        if (b.getClickingTogglesState())     return ButtonStyle::outline;
        return ButtonStyle::plain;
    }

    juce::Path rotaryTrack, rotaryValue, knobPointer, comboArrow;
};
