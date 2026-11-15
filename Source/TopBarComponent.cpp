#include "TopBarComponent.h"
#include "UIConstants.h"
#include <BinaryData.h>

namespace
{
    constexpr int kLogoW      = 150;
    constexpr int kArrowW     = 36;
    constexpr int kPresetW    = 280;
    constexpr int kIconBtnW   = 40;
    constexpr int kControlH   = 40;
    constexpr int kReadoutW   = 190;
    constexpr int kLedW       = 46;
    constexpr int kViewW      = 132;
    constexpr int kMeterW     = 12;

    const juce::Colour kGateOn  { 0xff44ff88 };
    const juce::Colour kHitOn   { 0xffffa500 };

    constexpr float kMeterFloorDb = -48.0f;
    constexpr int   kMeterSegments = 16;
}

TopBarComponent::TopBarComponent (JQGunkAudioProcessor& p)
    : processor (p)
{
    if (auto svgXml = juce::parseXML (juce::String::fromUTF8 (BinaryData::logo_svg, BinaryData::logo_svgSize)))
        logo = juce::Drawable::createFromSVG (*svgXml);

    presetCombo.setJustificationType (juce::Justification::centred);
    BassLookAndFeel::setComboFontHeight (presetCombo, UIConst::px (20.0f));
    presetCombo.setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    presetCombo.setTextWhenNothingSelected ("No preset");
    presetCombo.onChange = [this]
    {
        const int idx = presetCombo.getSelectedItemIndex();
        if (idx >= 0)
            loadPresetAt (idx);
    };

    prevBtn.setTooltip ("Previous preset");
    nextBtn.setTooltip ("Next preset");
    saveBtn.setTooltip ("Save preset");
    deleteBtn.setTooltip ("Delete user preset");
    prevBtn.onClick   = [this] { loadPresetAt (processor.getPresetManager().getAdjacentIndex (-1)); };
    nextBtn.onClick   = [this] { loadPresetAt (processor.getPresetManager().getAdjacentIndex (+1)); };
    saveBtn.onClick   = [this] { onSavePreset(); };
    deleteBtn.onClick = [this]
    {
        if (processor.getPresetManager().deleteUserPreset (presetCombo.getSelectedItemIndex()))
            refreshPresetCombo();
    };

    for (auto* c : std::initializer_list<juce::Component*> { &prevBtn, &presetCombo, &nextBtn, &saveBtn, &deleteBtn })
        addAndMakeVisible (c);

    viewSwitch.onChange = [this] (int idx) { if (onViewChanged) onViewChanged (idx == 1); };
    addAndMakeVisible (viewSwitch);

    refreshPresetCombo();
}

//==============================================================================
juce::String TopBarComponent::noteName (float hz)
{
    if (hz <= 0.0f) return "--";
    static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const int midi = juce::roundToInt (69.0f + 12.0f * std::log2 (hz / 440.0f));
    if (midi < 0) return "--";
    return juce::String (names[midi % 12]) + juce::String (midi / 12 - 1);
}

void TopBarComponent::update (bool isGateOpen, bool transientFired)
{
    const float hz = processor.getDetectedFrequency();
    const auto newNote = noteName (hz);
    const auto newHz   = hz > 0.0f ? juce::String (hz, 1) + " Hz" : juce::String ("-- Hz");

    hitHoldTicks = transientFired ? 5 : juce::jmax (0, hitHoldTicks - 1);

    // Meter: instant attack, ~20 dB/s fall
    const float peak = processor.consumeOutputPeak();
    const float newLevel = juce::jmax (peak, meterLevel * 0.85f);

    const bool changed = newNote != noteText || newHz != hzText || isGateOpen != gateOpen
                         || std::abs (newLevel - meterLevel) > 1.0e-4f || transientFired || hitHoldTicks == 1;
    noteText   = newNote;
    hzText     = newHz;
    gateOpen   = isGateOpen;
    meterLevel = newLevel < 1.0e-5f ? 0.0f : newLevel;

    const int cur = processor.getPresetManager().getCurrentIndex();
    if (presetCombo.getSelectedItemIndex() != cur)
    {
        presetCombo.setSelectedItemIndex (cur, juce::dontSendNotification);
        updateDeleteEnabled();
    }

    if (changed)
        repaint (readoutBounds.getUnion (meterBounds).getUnion (gateLedBounds).getUnion (hitLedBounds));
}

//==============================================================================
void TopBarComponent::paint (juce::Graphics& g)
{
    // Bar background and hairline
    g.setColour (BassLookAndFeel::headerBar);
    g.fillRect (getLocalBounds());
    g.setColour (BassLookAndFeel::border);
    g.fillRect (0, getHeight() - 1, getWidth(), 1);

    if (logo != nullptr)
        logo->drawWithin (g, logoBounds.toFloat(), juce::RectanglePlacement::centred, 1.0f);

    g.setColour (BassLookAndFeel::textDim);
    g.setFont (BassLookAndFeel::uiFont (UIConst::fontCaption));
    g.drawText ("v" JucePlugin_VersionString, versionBounds, juce::Justification::bottomLeft);

    // Preset box frame around < combo >
    {
        const auto b = presetBoxBounds.toFloat().reduced (0.5f);
        g.setColour (BassLookAndFeel::surfaceDark);
        g.fillRoundedRectangle (b, UIConst::buttonCornerRadius);
        g.setColour (BassLookAndFeel::border);
        g.drawRoundedRectangle (b, UIConst::buttonCornerRadius, 1.0f);
        g.fillRect ((float) presetCombo.getX(), b.getY(), 1.0f, b.getHeight());
        g.fillRect ((float) presetCombo.getRight() - 1.0f, b.getY(), 1.0f, b.getHeight());
    }

    // Note + Hz readout
    {
        auto r = readoutBounds;
        g.setColour (BassLookAndFeel::accent);
        g.setFont (BassLookAndFeel::uiFont (UIConst::px (32.0f), true));
        const int noteW = 56;
        g.drawText (noteText, r.removeFromLeft (noteW), juce::Justification::centredLeft);
        g.setFont (BassLookAndFeel::uiFont (UIConst::px (26.0f), true));
        g.setColour (hzText.startsWith ("--") ? BassLookAndFeel::textDim : BassLookAndFeel::accent);
        g.drawText (hzText, r, juce::Justification::centredLeft);
    }

    drawLed (g, gateLedBounds, gateOpen,         kGateOn, "GATE");
    drawLed (g, hitLedBounds,  hitHoldTicks > 0, kHitOn,  "HIT");

    // Output meter: segments from the floor up, green -> yellow (-18 dB) -> red (-6 dB)
    {
        const auto b = meterBounds.toFloat();
        g.setColour (BassLookAndFeel::surfaceDark);
        g.fillRoundedRectangle (b, 2.0f);

        const float db = juce::Decibels::gainToDecibels (meterLevel, kMeterFloorDb);
        const float frac = juce::jlimit (0.0f, 1.0f, (db - kMeterFloorDb) / -kMeterFloorDb);
        const auto inner = b.reduced (2.0f);
        const float segH = inner.getHeight() / (float) kMeterSegments;
        for (int i = 0; i < kMeterSegments; ++i)
        {
            const float segTopDb = kMeterFloorDb * (1.0f - (float) (i + 1) / kMeterSegments);
            const juce::Colour col = segTopDb > -6.0f  ? juce::Colour (0xffff4040)
                                   : segTopDb > -18.0f ? BassLookAndFeel::accent
                                                       : juce::Colour (0xff44dd66);
            const bool lit = (float) i < frac * kMeterSegments;
            const auto seg = juce::Rectangle<float> (inner.getX(), inner.getBottom() - segH * (float) (i + 1),
                                                     inner.getWidth(), segH).reduced (0.0f, 0.75f);
            g.setColour (lit ? col : col.withAlpha (0.12f));
            g.fillRect (seg);
        }
    }
}

void TopBarComponent::drawLed (juce::Graphics& g, juce::Rectangle<int> area, bool on,
                               juce::Colour onCol, const juce::String& label)
{
    auto a = area;
    auto ledArea = a.removeFromTop (a.getHeight() / 2).toFloat();
    const auto led = ledArea.withSizeKeepingCentre (10.0f, 10.0f).translated (0.0f, 3.0f);
    if (on)
    {
        g.setColour (onCol.withAlpha (0.25f));
        g.fillEllipse (led.expanded (3.5f));
    }
    g.setColour (on ? onCol : onCol.withMultipliedBrightness (0.25f).withMultipliedSaturation (0.6f));
    g.fillEllipse (led);

    g.setColour (on ? BassLookAndFeel::text : BassLookAndFeel::textDim);
    g.setFont (BassLookAndFeel::uiFont (UIConst::fontCaption, true));
    g.drawText (label, a, juce::Justification::centredTop);
}

void TopBarComponent::resized()
{
    auto r = getLocalBounds().reduced (UIConst::editorPad, 0);
    const int cy = r.getCentreY();
    auto rowOf = [cy] (juce::Rectangle<int> col, int h) { return col.withHeight (h).withCentre ({ col.getCentreX(), cy }); };

    // The drawable's bounds are much larger than the lettering (splatter, drips), so draw it
    // well beyond the bar, as the old header did: the letters end up ~45 px tall and ~120 wide.
    {
        constexpr int logoH = 118;
        constexpr int logoW = logoH * 184 / 98;          // SVG aspect
        const auto col = r.removeFromLeft (kLogoW);
        logoBounds = { col.getX() + 60 - logoW / 2, (getHeight() - logoH) * 6 / 10, logoW, logoH };
        versionBounds = { col.getX() + 124, 0, 60, getHeight() - 12 };
    }

    // Right cluster, from the right edge inwards
    meterBounds = rowOf (r.removeFromRight (kMeterW), kControlH + 4);
    r.removeFromRight (14);
    viewSwitch.setBounds (rowOf (r.removeFromRight (kViewW), 34));
    r.removeFromRight (14);
    hitLedBounds  = rowOf (r.removeFromRight (kLedW), kControlH);
    gateLedBounds = rowOf (r.removeFromRight (kLedW), kControlH);
    r.removeFromRight (8);
    readoutBounds = rowOf (r.removeFromRight (kReadoutW), kControlH);

    // Preset group, centred in what's left
    const int groupW = kArrowW * 2 + kPresetW + 10 + kIconBtnW * 2 + 6;
    auto group = rowOf (r.withSizeKeepingCentre (juce::jmin (groupW, r.getWidth()), r.getHeight()), kControlH);
    presetBoxBounds = group.removeFromLeft (kArrowW * 2 + kPresetW);
    {
        auto box = presetBoxBounds;
        prevBtn.setBounds (box.removeFromLeft (kArrowW).reduced (2));
        nextBtn.setBounds (box.removeFromRight (kArrowW).reduced (2));
        presetCombo.setBounds (box.reduced (1, 1));
    }
    group.removeFromLeft (10);
    saveBtn.setBounds (group.removeFromLeft (kIconBtnW));
    group.removeFromLeft (6);
    deleteBtn.setBounds (group.removeFromLeft (kIconBtnW));
}

//==============================================================================
void TopBarComponent::refreshPresetCombo()
{
    presetCombo.clear (juce::dontSendNotification);
    const auto& presets = processor.getPresetManager().getPresets();
    bool lastWasFactory = true;
    int item = 1;
    for (const auto& p : presets)
    {
        if (lastWasFactory && ! p.isFactory)
            presetCombo.addSectionHeading ("User Presets");
        presetCombo.addItem (p.name, item++);
        lastWasFactory = p.isFactory;
    }
    presetCombo.setSelectedItemIndex (processor.getPresetManager().getCurrentIndex(), juce::dontSendNotification);
    updateDeleteEnabled();
}

void TopBarComponent::updateDeleteEnabled()
{
    const auto& presets = processor.getPresetManager().getPresets();
    const int cur = processor.getPresetManager().getCurrentIndex();
    deleteBtn.setEnabled (cur >= 0 && cur < (int) presets.size() && ! presets[(size_t) cur].isFactory);
}

void TopBarComponent::loadPresetAt (int index)
{
    if (index < 0) return;
    if (! processor.getPresetManager().loadPreset (index))
        return;

    auto missing = processor.syncOscillatorAfterPresetLoad();
    if (missing.size() > 0)
    {
        juce::String msg = "The following files referenced by this preset could not be found "
                           "and have been replaced with default waveforms:\n\n";
        for (auto& m : missing)
            msg += juce::String (juce::CharPointer_UTF8 ("\xe2\x80\xa2 ")) + m + "\n";
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                "Missing Files", msg.trimEnd());
    }

    presetCombo.setSelectedItemIndex (index, juce::dontSendNotification);
    updateDeleteEnabled();
}

void TopBarComponent::onSavePreset()
{
    auto& mgr = processor.getPresetManager();
    const int cur = mgr.getCurrentIndex();
    juce::String defaultName = (cur >= 0 && cur < (int) mgr.getPresets().size())
                               ? mgr.getPresets()[(size_t) cur].name : "New Preset";

    auto* aw = new juce::AlertWindow ("Save Preset", "Enter preset name:",
                                      juce::MessageBoxIconType::NoIcon);
    aw->addTextEditor ("presetName", defaultName, {});
    aw->addButton ("Save",   1, juce::KeyPress (juce::KeyPress::returnKey));
    aw->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    juce::Component::SafePointer<TopBarComponent> safeThis (this);
    aw->enterModalState (true,
        juce::ModalCallbackFunction::create ([safeThis, aw] (int result)
        {
            if (safeThis == nullptr || result != 1) return;
            const juce::String name = aw->getTextEditorContents ("presetName").trim();
            if (name.isEmpty()) return;
            auto& m = safeThis->processor.getPresetManager();
            safeThis->processor.saveUserPreset (name);
            m.rescanPresets();
            const auto& presets = m.getPresets();
            for (int i = 0; i < (int) presets.size(); ++i)
                if (presets[(size_t) i].name == name && ! presets[(size_t) i].isFactory)
                    { m.setCurrentIndex (i); break; }
            safeThis->refreshPresetCombo();
        }),
        true /* deleteWhenDismissed */);
}
