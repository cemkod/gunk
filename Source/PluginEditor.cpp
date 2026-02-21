#include "PluginEditor.h"

//==============================================================================
BassSynthAudioProcessorEditor::BassSynthAudioProcessorEditor (BassSynthAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      levelAttach    (p.apvts, "level",     levelSlider),
      mixAttach      (p.apvts, "mix",       mixSlider)
{
    auto setupSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& name)
    {
        s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
        addAndMakeVisible (s);

        l.setText (name, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (l);
    };

    setupSlider (levelSlider,     levelLabel,     "Level");
    setupSlider (mixSlider,       mixLabel,       "Mix");

    setSize (340, 180);
}

BassSynthAudioProcessorEditor::~BassSynthAudioProcessorEditor() {}

//==============================================================================
void BassSynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1a1a2e));

    g.setColour (juce::Colour (0xffe94560));
    g.setFont (juce::Font (18.0f, juce::Font::bold));
    g.drawFittedText ("Bass Synth", getLocalBounds().removeFromTop (40),
                      juce::Justification::centred, 1);
}

void BassSynthAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (10);
    area.removeFromTop (40); // title

    const int knobWidth = area.getWidth() / 3;

    auto labelArea = area.removeFromBottom (24);
    auto knobArea  = area;

    levelLabel    .setBounds (labelArea.removeFromLeft (knobWidth));
    mixLabel      .setBounds (labelArea.removeFromLeft (knobWidth));

    levelSlider    .setBounds (knobArea.removeFromLeft (knobWidth));
    mixSlider      .setBounds (knobArea.removeFromLeft (knobWidth));
}
