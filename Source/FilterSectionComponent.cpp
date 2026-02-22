#include "FilterSectionComponent.h"
#include "LookAndFeel.h"

FilterSectionComponent::FilterSectionComponent (juce::AudioProcessorValueTreeState& avts)
    : apvts (avts),
      sensitivityAttach (avts, "envSensitivity", sensitivitySlider),
      resonanceAttach   (avts, "envResonance",   resonanceSlider),
      decayAttach       (avts, "envDecay",        decaySlider)
{
    auto setupSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& name)
    {
        s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
        s.setColour (juce::Slider::textBoxTextColourId,       BassLookAndFeel::text);
        s.setColour (juce::Slider::textBoxBackgroundColourId, BassLookAndFeel::surfaceDark);
        s.setColour (juce::Slider::textBoxOutlineColourId,    BassLookAndFeel::borderDim);
        addAndMakeVisible (s);

        l.setText (name, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.setFont (juce::Font (11.0f, juce::Font::bold));
        l.setColour (juce::Label::textColourId, BassLookAndFeel::text);
        addAndMakeVisible (l);
    };

    setupSlider (sensitivitySlider, sensitivityLabel, "SENS");
    setupSlider (resonanceSlider,   resonanceLabel,   "RESO");
    setupSlider (decaySlider,       decayLabel,       "DECAY");

    // Sweep buttons
    const int sweepGroup = 102;
    for (auto* b : { &sweepBtnOff, &sweepBtnUp, &sweepBtnDown })
    {
        b->setRadioGroupId (sweepGroup);
        b->setClickingTogglesState (true);
        addAndMakeVisible (b);
    }
    sweepBtnOff .onClick = [this] { setSweepParam (0); };
    sweepBtnUp  .onClick = [this] { setSweepParam (1); };
    sweepBtnDown.onClick = [this] { setSweepParam (2); };

    updateButtonStates();
}

FilterSectionComponent::~FilterSectionComponent()
{
    setLookAndFeel (nullptr);
}

void FilterSectionComponent::setSweepParam (int idx)
{
    auto* p = dynamic_cast<juce::AudioParameterChoice*> (
        apvts.getParameter ("sweepMode"));
    if (p) *p = idx;
}

void FilterSectionComponent::updateButtonStates()
{
    const int idx = (int) apvts.getRawParameterValue ("sweepMode")->load();
    sweepBtnOff .setToggleState (idx == 0, juce::dontSendNotification);
    sweepBtnUp  .setToggleState (idx == 1, juce::dontSendNotification);
    sweepBtnDown.setToggleState (idx == 2, juce::dontSendNotification);
}

void FilterSectionComponent::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds();
    g.setColour (BassLookAndFeel::surface);
    g.fillRoundedRectangle (bounds.toFloat(), 6.0f);
    g.setColour (BassLookAndFeel::border);
    g.drawRoundedRectangle (bounds.toFloat(), 6.0f, 1.0f);
    g.setColour (BassLookAndFeel::text);
    g.setFont (juce::Font (11.0f, juce::Font::bold));
    g.drawText ("ENV FILTER", bounds.reduced (6, 4).removeFromTop (14), juce::Justification::topLeft);
}

void FilterSectionComponent::resized()
{
    auto inner = getLocalBounds().reduced (8);
    inner.removeFromTop (18); // skip section label row

    // Sweep button row
    auto sweepRow = inner.removeFromTop (24);
    const int btnW = sweepRow.getWidth() / 3;
    sweepBtnOff .setBounds (sweepRow.removeFromLeft (btnW));
    sweepBtnUp  .setBounds (sweepRow.removeFromLeft (btnW));
    sweepBtnDown.setBounds (sweepRow);

    inner.removeFromTop (10);

    // Sensitivity | Resonance | Decay knobs
    const int knobW = inner.getWidth() / 3;
    auto knobRow = inner.removeFromTop (75);
    sensitivitySlider.setBounds (knobRow.removeFromLeft (knobW));
    resonanceSlider  .setBounds (knobRow.removeFromLeft (knobW));
    decaySlider      .setBounds (knobRow.removeFromLeft (knobW));

    auto lblRow = inner.removeFromTop (18);
    sensitivityLabel.setBounds (lblRow.removeFromLeft (knobW));
    resonanceLabel  .setBounds (lblRow.removeFromLeft (knobW));
    decayLabel      .setBounds (lblRow.removeFromLeft (knobW));
}
