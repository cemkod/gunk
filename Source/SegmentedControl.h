#pragma once
#include <JuceHeader.h>
#include "LookAndFeel.h"
#include "UIConstants.h"

// A row of mutually exclusive options drawn as one pill, the selected segment filled in
// accent. Either drive it yourself (setSelected / onChange) or bind it to an
// AudioParameterChoice with attachToParameter(), which sends one complete gesture per click.
class SegmentedControl : public juce::Component
{
public:
    explicit SegmentedControl (juce::StringArray segmentLabels = {})
        : labels (std::move (segmentLabels))
    {
        setRepaintsOnMouseActivity (true);
        setWantsKeyboardFocus (false);
    }

    std::function<void (int)> onChange;

    void setLabels (juce::StringArray l) { labels = std::move (l); repaint(); }

    int  getSelected() const { return selected; }
    void setSelected (int idx, juce::NotificationType n = juce::sendNotificationSync)
    {
        idx = juce::jlimit (0, juce::jmax (0, labels.size() - 1), idx);
        if (idx == selected) return;
        selected = idx;
        repaint();
        if (n != juce::dontSendNotification)
        {
            if (attachment != nullptr) attachment->setValueAsCompleteGesture ((float) idx);
            if (onChange) onChange (idx);
        }
    }

    // Binds to a choice parameter: index i <-> segment i.
    void attachToParameter (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID)
    {
        auto* p = apvts.getParameter (paramID);
        jassert (p != nullptr);
        attachment = std::make_unique<juce::ParameterAttachment> (
            *p, [this] (float v) { setSelected (juce::roundToInt (v), juce::dontSendNotification); });
        attachment->sendInitialUpdate();
    }

    void paint (juce::Graphics& g) override
    {
        const int n = labels.size();
        if (n == 0) return;

        const auto b = getLocalBounds().toFloat().reduced (0.5f);
        const float r = UIConst::buttonCornerRadius;

        g.setColour (BassLookAndFeel::surfaceDark);
        g.fillRoundedRectangle (b, r);

        const float segW = b.getWidth() / (float) n;
        const int hover = isMouseOver() ? segmentAt (getMouseXYRelative().x) : -1;

        for (int i = 0; i < n; ++i)
        {
            auto seg = juce::Rectangle<float> (b.getX() + segW * (float) i, b.getY(), segW, b.getHeight());
            if (i == selected)
            {
                g.setColour (isEnabled() ? BassLookAndFeel::accent : BassLookAndFeel::iconDim);
                g.fillRoundedRectangle (seg.reduced (2.0f), r - 1.0f);
            }
            else if (i == hover && isEnabled())
            {
                g.setColour (BassLookAndFeel::buttonFill);
                g.fillRoundedRectangle (seg.reduced (2.0f), r - 1.0f);
            }

            if (i > 0 && i != selected && i - 1 != selected)
            {
                g.setColour (BassLookAndFeel::border);
                g.fillRect (seg.getX() - 0.5f, seg.getY() + 6.0f, 1.0f, seg.getHeight() - 12.0f);
            }

            g.setColour (i == selected ? BassLookAndFeel::accentText
                                       : (isEnabled() ? BassLookAndFeel::text.withAlpha (0.8f) : BassLookAndFeel::iconDim));
            g.setFont (BassLookAndFeel::uiFont (UIConst::fontButtonText, true));
            g.drawFittedText (labels[i], seg.toNearestInt().reduced (4, 0), juce::Justification::centred, 1);
        }

        g.setColour (BassLookAndFeel::border);
        g.drawRoundedRectangle (b, r, 1.0f);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (isEnabled())
            setSelected (segmentAt (e.x));
    }

    void mouseMove (const juce::MouseEvent&) override { repaint(); }

private:
    int segmentAt (int x) const
    {
        const int n = juce::jmax (1, labels.size());
        return juce::jlimit (0, n - 1, x * n / juce::jmax (1, getWidth()));
    }

    juce::StringArray labels;
    int selected = 0;
    std::unique_ptr<juce::ParameterAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SegmentedControl)
};

// A dim caption followed by a SegmentedControl, e.g. "AMP [Input | ADSR]" in a header.
class CaptionedSegmentedControl : public juce::Component
{
public:
    CaptionedSegmentedControl (const juce::String& captionText, juce::StringArray labels, int captionWidth)
        : control (std::move (labels)), captionW (captionWidth)
    {
        BassLookAndFeel::setupCaption (caption, captionText, *this, juce::Justification::centredRight);
        addAndMakeVisible (control);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        caption.setBounds (b.removeFromLeft (captionW));
        b.removeFromLeft (8);
        control.setBounds (b);
    }

    juce::Label      caption;
    SegmentedControl control;

private:
    int captionW;
};
