#include "PluginEditor.h"
#include <BinaryData.h>

//==============================================================================
JQGunkAudioProcessorEditor::JQGunkAudioProcessorEditor (JQGunkAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      gateSection   (p.apvts),
      oscSection    (p, p.apvts),
      filterSection (p.apvts)
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

    startTimerHz (30);

    if (auto svgXml = juce::parseXML (juce::String::fromUTF8 (BinaryData::logo_svg, BinaryData::logo_svgSize)))
        logo = juce::Drawable::createFromSVG (*svgXml);

    setSize (340, 670);
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
}

//==============================================================================
void JQGunkAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (BassLookAndFeel::bg);

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

    gateSection.setBounds   (area.removeFromTop (137));
    area.removeFromTop (8);
    oscSection.setBounds    (area.removeFromTop (272));
    area.removeFromTop (8);
    filterSection.setBounds (area.removeFromTop (161));
}
