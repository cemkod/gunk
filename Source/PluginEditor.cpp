#include "PluginEditor.h"
#include "UIConstants.h"
#include <BinaryData.h>

//==============================================================================
JQGunkAudioProcessorEditor::JQGunkAudioProcessorEditor (JQGunkAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      gateSection   (p.apvts),
      filterSection (p, p.apvts),
      subOscSection (p.apvts),
      oscSection  ("OSC",   OscParamIds { "waveform",     "oscLevel",   "unisonVoices",    "unisonDetune",    "unisonBlend",    "octaveShift"    }, p.apvts),
      osc2Section ("OSC 2", OscParamIds { "osc2Waveform", "osc2Level",  "osc2UnisonVoices","osc2UnisonDetune","osc2UnisonBlend","osc2OctaveShift" }, p.apvts),
      transientSection (p.apvts)
{
    // Wire OSC 1 wavetable callbacks
    oscSection.isCustomWavetableLoaded   = [&p] { return p.isCustomWavetableLoaded(); };
    oscSection.isCustomWaveformActive    = [&p] { return p.isCustomWaveformActive(); };
    oscSection.reactivateCustomWavetable = [&p] { p.reactivateCustomWavetable(); };
    oscSection.loadWavetableFromFile     = [&p] (const juce::File& f) { return p.loadWavetableFromFile (f); };

    // Wire OSC 2 wavetable callbacks
    osc2Section.isCustomWavetableLoaded   = [&p] { return p.isCustomWavetable2Loaded(); };
    osc2Section.isCustomWaveformActive    = [&p] { return p.isCustomWaveform2Active(); };
    osc2Section.reactivateCustomWavetable = [&p] { p.reactivateCustomWavetable2(); };
    osc2Section.loadWavetableFromFile     = [&p] (const juce::File& f) { return p.loadWavetable2FromFile (f); };

    addAndMakeVisible (gateSection);
    addAndMakeVisible (filterSection);
    addAndMakeVisible (subOscSection);
    addAndMakeVisible (oscSection);
    addAndMakeVisible (osc2Section);
    transientSection.loadSampleFromFile = [&p] (const juce::File& f) { return p.loadTransientSampleFromFile (f); };
    transientSection.getLoadedPath      = [&p] { return p.getTransientSamplePath(); };
    addAndMakeVisible (transientSection);

    // Set LookAndFeel after sections are children so propagation covers them
    setLookAndFeel (&lookAndFeel);

    freqLabel.setFont (juce::Font (UIConst::uiFontSize));
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

    startTimerHz (UIConst::uiTimerHz);

    if (auto svgXml = juce::parseXML (juce::String::fromUTF8 (BinaryData::logo_svg, BinaryData::logo_svgSize)))
        logo = juce::Drawable::createFromSVG (*svgXml);

    setSize (UIConst::editorWidth, UIConst::editorHeight);
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
    osc2Section.updateButtonStates();
    subOscSection.updateButtonStates();
    transientSection.updateButtonStates();
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

    const bool newTransient = processor.consumeTransient();
    if (newTransient != transientLedActive)
    {
        transientLedActive = newTransient;
        repaint (transientLedBounds);
    }

    gateSection.setMeterValues (processor.getEnvelope(), nowOpen);

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
        const juce::Colour ledOn  { 0xff44ff88 }; // matches kGateLedOnColor in GateSectionComponent.cpp
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

    // Transient LED (amber)
    {
        const auto bounds = transientLedBounds.toFloat();
        const juce::Colour ledOn  { 0xffffa500 };
        const juce::Colour ledOff { 0xff332200 };
        const juce::Colour col = transientLedActive ? ledOn : ledOff;

        if (transientLedActive)
        {
            g.setColour (ledOn.withAlpha (0.25f));
            g.fillEllipse (bounds.expanded (3.0f));
        }

        g.setColour (col);
        g.fillEllipse (bounds);

        g.setColour (juce::Colours::white.withAlpha (transientLedActive ? 0.35f : 0.08f));
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
    auto area = getLocalBounds().reduced (UIConst::editorPad);
    auto header = area.removeFromTop (UIConst::headerHeight);
    auto labelRow = header.removeFromBottom (UIConst::labelRowH);
    gateLedBounds = labelRow.removeFromLeft (UIConst::gateLedSize + UIConst::gateLedPad)
                            .withSizeKeepingCentre (UIConst::gateLedSize, UIConst::gateLedSize);
    transientLedBounds = gateLedBounds.translated (gateLedBounds.getWidth() + 4, 0);
    freqLabel.setBounds (labelRow);
    area.removeFromTop (4);  // gap below title

    auto strip = area.removeFromTop (UIConst::presetStripH);
    presetStripBounds = strip;
    presetSaveBtn.setBounds   (strip.removeFromLeft (40));
    strip.removeFromLeft (4);
    presetDeleteBtn.setBounds (strip.removeFromRight (36));
    strip.removeFromRight (4);
    presetCombo.setBounds (strip);
    area.removeFromTop (4);  // gap after strip

    // Top row: gate (left half) + filter (right half), equal widths, topRowH tall
    auto topRow = area.removeFromTop (UIConst::topRowH);
    const int halfW = topRow.getWidth() / 2;
    gateSection  .setBounds (topRow.removeFromLeft (halfW).withTrimmedRight (UIConst::sectionGap / 2));
    filterSection.setBounds (topRow.withTrimmedLeft (UIConst::sectionGap / 2));

    area.removeFromTop (UIConst::sectionGap);

    // Osc row: 4 equal columns
    auto oscRow = area.removeFromTop (UIConst::oscSectionH);
    subOscSection.setBounds (oscRow.removeFromLeft (UIConst::oscColW).withTrimmedRight (UIConst::sectionGap / 2));
    oscSection   .setBounds (oscRow.removeFromLeft (UIConst::oscColW).withTrimmedLeft (UIConst::sectionGap / 2).withTrimmedRight (UIConst::sectionGap / 2));
    osc2Section  .setBounds (oscRow.removeFromLeft (UIConst::oscColW).withTrimmedLeft (UIConst::sectionGap / 2).withTrimmedRight (UIConst::sectionGap / 2));
    transientSection.setBounds (oscRow.withTrimmedLeft (UIConst::sectionGap / 2));
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
