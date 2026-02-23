#include "PluginEditor.h"
#include <BinaryData.h>

//==============================================================================
JQGunkAudioProcessorEditor::JQGunkAudioProcessorEditor (JQGunkAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      gateSection   (p.apvts),
      oscSection    (p, p.apvts),
      filterSection (p, p.apvts)
{
    addAndMakeVisible (gateSection);
    addAndMakeVisible (oscSection);
    addAndMakeVisible (filterSection);

    // Set LookAndFeel after sections are children so propagation covers them
    setLookAndFeel (&lookAndFeel);

    freqLabel.setFont (juce::Font (11.0f));
    freqLabel.setColour (juce::Label::textColourId, BassLookAndFeel::text.withAlpha (0.7f));
    freqLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (freqLabel);

    // Preset strip setup
    refreshPresetCombo();
    presetCombo.onChange = [this]
    {
        const int idx = presetCombo.getSelectedItemIndex();
        if (idx >= 0)
        {
            processor.getPresetManager().loadPreset (idx);
            processor.syncOscillatorAfterPresetLoad();
            const auto& presets = processor.getPresetManager().getPresets();
            presetDeleteBtn.setEnabled (idx < (int) presets.size() && ! presets[(size_t) idx].isFactory);
        }
    };
    presetSaveBtn.onClick   = [this] { onSavePreset(); };
    presetDeleteBtn.onClick = [this]
    {
        const int idx = presetCombo.getSelectedItemIndex();
        if (processor.getPresetManager().deleteUserPreset (idx))
        {
            refreshPresetCombo();
            presetDeleteBtn.setEnabled (false);
        }
    };

    addAndMakeVisible (presetCombo);
    addAndMakeVisible (presetSaveBtn);
    addAndMakeVisible (presetDeleteBtn);

    startTimerHz (30);

    if (auto svgXml = juce::parseXML (juce::String::fromUTF8 (BinaryData::logo_svg, BinaryData::logo_svgSize)))
        logo = juce::Drawable::createFromSVG (*svgXml);

    setSize (340, 800);
}

JQGunkAudioProcessorEditor::~JQGunkAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
    stopTimer();
}

//==============================================================================
void JQGunkAudioProcessorEditor::timerCallback()
{
    oscSection.updateButtonStates();
    filterSection.updateButtonStates();
    filterSection.repaintDisplay();

    const float freq = processor.getDetectedFrequency();
    if (freq > 0.0f)
        freqLabel.setText (juce::String (freq, 1) + " Hz", juce::dontSendNotification);
    else
        freqLabel.setText ("-- Hz", juce::dontSendNotification);

    const bool nowOpen = processor.isGateOpen();
    if (nowOpen != gateOpen)
    {
        gateOpen = nowOpen;
        repaint (gateLedBounds);
    }

    const int cur = processor.getPresetManager().getCurrentIndex();
    if (presetCombo.getSelectedItemIndex() != cur)
        presetCombo.setSelectedItemIndex (cur, juce::dontSendNotification);
}

//==============================================================================
void JQGunkAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (BassLookAndFeel::bg);

    g.setColour (BassLookAndFeel::surface);
    g.fillRoundedRectangle (presetStripBounds.toFloat(), 4.0f);
    g.setColour (BassLookAndFeel::border);
    g.drawRoundedRectangle (presetStripBounds.toFloat(), 4.0f, 1.0f);

    // Gate LED
    {
        const auto bounds = gateLedBounds.toFloat();
        const juce::Colour ledOn  { 0xff44ff88 };
        const juce::Colour ledOff { 0xff223322 };
        const juce::Colour col = gateOpen ? ledOn : ledOff;

        // Outer glow when open
        if (gateOpen)
        {
            g.setColour (ledOn.withAlpha (0.25f));
            g.fillEllipse (bounds.expanded (3.0f));
        }

        g.setColour (col);
        g.fillEllipse (bounds);

        // Highlight specular
        g.setColour (juce::Colours::white.withAlpha (gateOpen ? 0.35f : 0.08f));
        g.fillEllipse (bounds.reduced (1.5f).withHeight (bounds.getHeight() * 0.45f));
    }
}

void JQGunkAudioProcessorEditor::paintOverChildren (juce::Graphics& g)
{
    if (logo != nullptr)
    {
        constexpr float logoH = 140.0f;
        constexpr float logoW = logoH * (184.0f / 98.0f);
        auto headerBounds = getLocalBounds().removeFromTop (60).toFloat();
        juce::Rectangle<float> logoBounds (
            (headerBounds.getWidth() - logoW) * 0.5f,
            (headerBounds.getHeight() - logoH) * 0.6f,
            logoW, logoH);
        logo->drawWithin (g, logoBounds, juce::RectanglePlacement::centred, 1.0f);
    }
}

void JQGunkAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (10);
    auto header = area.removeFromTop (60);
    auto labelRow = header.removeFromBottom (16);
    const int ledSize = 8;
    gateLedBounds = labelRow.removeFromLeft (ledSize + 6)
                            .withSizeKeepingCentre (ledSize, ledSize);
    freqLabel.setBounds (labelRow);
    area.removeFromTop (4);  // gap below title

    auto strip = area.removeFromTop (28);
    presetStripBounds = strip;
    presetSaveBtn.setBounds   (strip.removeFromLeft (40));
    strip.removeFromLeft (4);
    presetDeleteBtn.setBounds (strip.removeFromRight (36));
    strip.removeFromRight (4);
    presetCombo.setBounds (strip);
    area.removeFromTop (4);  // gap after strip

    gateSection.setBounds   (area.removeFromTop (137));
    area.removeFromTop (8);
    oscSection.setBounds    (area.removeFromTop (272));
    area.removeFromTop (8);
    filterSection.setBounds (area.removeFromTop (261));
}

//==============================================================================
void JQGunkAudioProcessorEditor::refreshPresetCombo()
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
    const int cur = processor.getPresetManager().getCurrentIndex();
    presetCombo.setSelectedItemIndex (cur, juce::dontSendNotification);
    const bool isUserPreset = cur >= 0 && cur < (int) presets.size()
                              && ! presets[(size_t) cur].isFactory;
    presetDeleteBtn.setEnabled (isUserPreset);
}

void JQGunkAudioProcessorEditor::onSavePreset()
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

    aw->enterModalState (true,
        juce::ModalCallbackFunction::create ([this, aw] (int result)
        {
            if (result == 1)
            {
                const juce::String name = aw->getTextEditorContents ("presetName").trim();
                if (name.isEmpty()) return;
                auto& m = processor.getPresetManager();
                m.saveUserPreset (name);
                m.rescanPresets();
                const auto& presets = m.getPresets();
                for (int i = 0; i < (int) presets.size(); ++i)
                    if (presets[(size_t) i].name == name && ! presets[(size_t) i].isFactory)
                        { m.setCurrentIndex (i); break; }
                refreshPresetCombo();
            }
        }),
        true /* deleteWhenDismissed */);
}
