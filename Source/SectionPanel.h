#pragma once
#include <JuceHeader.h>
#include "LookAndFeel.h"
#include "UIConstants.h"

// Base for every section: rounded body, a header strip with the title, and two optional
// header slots on the right: a power toggle (far right) and one more component left of it
// (a mode combo, a segmented control, a caption...).
//
// Subclasses lay out their controls in layoutBody(), which gets the padded area under the
// header. setDimmed() fades the body (not the header) when the section is bypassed.
class SectionPanel : public juce::Component
{
public:
    explicit SectionPanel (juce::String title)
        : sectionTitle (std::move (title)) {}

    void setTitle (const juce::String& t)     { sectionTitle = t; repaint(); }
    void setSuppressBorder (bool b)           { suppressBorder_ = b; }

    // The component must already be a child of this panel.
    void setHeaderPower (juce::Component* c)  { headerPower = c; relayout(); }
    void setHeaderRight (juce::Component* c, int width)
    {
        headerRight = c;
        headerRightW = width;
        relayout();
    }

    void setDimmed (bool shouldDim)
    {
        if (dimmed == shouldDim) return;
        dimmed = shouldDim;
        for (auto* c : getChildren())
            if (c != headerPower && c != headerRight && ! isHeaderChild (c))
                c->setAlpha (dimmed ? 0.45f : 1.0f);
        repaint();
    }
    bool isDimmed() const { return dimmed; }

    // Dims the body whenever this bool parameter (an *Enabled switch) is off.
    // Call refreshDimmed() periodically (the main view does, from the UI timer).
    void setEnabledParameter (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID)
    {
        enabledParam = apvts.getRawParameterValue (paramID);
        jassert (enabledParam != nullptr);
    }
    virtual void refreshDimmed()
    {
        if (enabledParam != nullptr)
            setDimmed (enabledParam->load (std::memory_order_relaxed) < 0.5f);
    }

    void paint (juce::Graphics& g) override
    {
        if (suppressBorder_) return;
        const auto bounds = getLocalBounds().toFloat();
        const float r = UIConst::sectionCornerRadius;

        g.setColour (BassLookAndFeel::surface);
        g.fillRoundedRectangle (bounds, r);

        // Header strip, clipped to the rounded top, with a hairline under it
        {
            juce::Graphics::ScopedSaveState ss (g);
            juce::Path clip;
            clip.addRoundedRectangle (bounds, r);
            g.reduceClipRegion (clip);
            g.setColour (BassLookAndFeel::headerBar);
            g.fillRect (bounds.withHeight ((float) UIConst::sectionHeaderH));
        }
        g.setColour (BassLookAndFeel::border);
        g.fillRect (0.0f, (float) UIConst::sectionHeaderH - 1.0f, bounds.getWidth(), 1.0f);

        g.setColour (BassLookAndFeel::border);
        g.drawRoundedRectangle (bounds.reduced (0.5f), r, 1.0f);

        g.setColour (BassLookAndFeel::accent);
        g.setFont (BassLookAndFeel::uiFont (UIConst::fontSectionTitle, true));
        g.drawText (sectionTitle, getTitleBounds(), juce::Justification::centredLeft);
    }

    void resized() override
    {
        auto header = getHeaderBounds().reduced (UIConst::sectionInnerPad, 0);
        if (headerPower != nullptr)
        {
            headerPower->setBounds (header.removeFromRight (UIConst::powerToggleW)
                                          .withSizeKeepingCentre (UIConst::powerToggleW, UIConst::powerToggleH));
            header.removeFromRight (UIConst::sectionInnerPad);
        }
        if (headerRight != nullptr)
            headerRight->setBounds (header.removeFromRight (headerRightW).reduced (0, 3));

        layoutBody (getBodyBounds());
    }

protected:
    // Lay out the section's controls inside `body` (already padded, below the header).
    virtual void layoutBody (juce::Rectangle<int> /*body*/) {}

    // Components the subclass places in the header itself (e.g. tab buttons); they are
    // not dimmed by setDimmed().
    virtual bool isHeaderChild (juce::Component*) const { return false; }

    juce::Rectangle<int> getHeaderBounds() const
    {
        return getLocalBounds().removeFromTop (UIConst::sectionHeaderH);
    }

    juce::Rectangle<int> getTitleBounds() const
    {
        return getHeaderBounds().reduced (UIConst::sectionInnerPad, 0);
    }

    juce::Rectangle<int> getBodyBounds() const
    {
        auto b = getLocalBounds();
        b.removeFromTop (UIConst::sectionHeaderH);
        return b.reduced (UIConst::sectionInnerPad);
    }

    const juce::String& getTitle() const { return sectionTitle; }

    void relayout() { if (! getLocalBounds().isEmpty()) resized(); }

private:
    juce::String sectionTitle;
    bool suppressBorder_ = false;
    bool dimmed = false;
    std::atomic<float>* enabledParam = nullptr;
    juce::Component* headerPower = nullptr;
    juce::Component* headerRight = nullptr;
    int headerRightW = 0;
};
