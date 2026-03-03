#include "ModMatrixComponent.h"
#include "LookAndFeel.h"
#include "UIConstants.h"

static const juce::StringArray kSourceNames { "None", "Envelope", "Pitch", "Mod Env", "LFO" };
static const juce::StringArray kTargetNames { "None", "Morph 1", "Morph 2", "Filter Freq",
                                               "Filter Res", "OSC 1 Level", "OSC 2 Level",
                                               "Unison 1 Detune", "Sub Level" };

ModMatrixComponent::ModMatrixComponent (juce::AudioProcessorValueTreeState& apvts)
{
    for (int i = 0; i < 8; ++i)
    {
        auto& row = rows_[(size_t) i];
        const juce::String n (i);

        // Source combo
        for (int j = 0; j < kSourceNames.size(); ++j)
            row.sourceCombo.addItem (kSourceNames[j], j + 1);
        row.sourceCombo.setColour (juce::ComboBox::backgroundColourId, BassLookAndFeel::surface);
        row.sourceCombo.setColour (juce::ComboBox::textColourId,       BassLookAndFeel::text);
        row.sourceCombo.setColour (juce::ComboBox::outlineColourId,    BassLookAndFeel::border);
        row.sourceAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
            apvts, "modSlot" + n + "Source", row.sourceCombo);
        addAndMakeVisible (row.sourceCombo);

        // Target combo
        for (int j = 0; j < kTargetNames.size(); ++j)
            row.targetCombo.addItem (kTargetNames[j], j + 1);
        row.targetCombo.setColour (juce::ComboBox::backgroundColourId, BassLookAndFeel::surface);
        row.targetCombo.setColour (juce::ComboBox::textColourId,       BassLookAndFeel::text);
        row.targetCombo.setColour (juce::ComboBox::outlineColourId,    BassLookAndFeel::border);
        row.targetAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
            apvts, "modSlot" + n + "Target", row.targetCombo);
        addAndMakeVisible (row.targetCombo);

        // Amount slider (linear horizontal, text box on right)
        row.amountSlider.setSliderStyle (juce::Slider::LinearHorizontal);
        row.amountSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 48, 20);
        row.amountSlider.setColour (juce::Slider::thumbColourId,             BassLookAndFeel::accent);
        row.amountSlider.setColour (juce::Slider::textBoxTextColourId,       BassLookAndFeel::textDim);
        row.amountSlider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        row.amountSlider.setColour (juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
        row.amountAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            apvts, "modSlot" + n + "Amount", row.amountSlider);
        addAndMakeVisible (row.amountSlider);
    }
}

ModMatrixComponent::~ModMatrixComponent()
{
    setLookAndFeel (nullptr);
}

void ModMatrixComponent::paint (juce::Graphics& g)
{
    g.fillAll (BassLookAndFeel::bg);

    auto inner = getLocalBounds().reduced (8);

    // Header background
    g.setColour (BassLookAndFeel::surfaceDark);
    g.fillRoundedRectangle (inner.removeFromTop (kHeaderH).toFloat(), 4.0f);

    // Alternating row shading
    auto rowArea = inner;
    for (int i = 0; i < 8; ++i)
    {
        auto row = rowArea.removeFromTop (kRowH);
        if (i % 2 == 1)
        {
            g.setColour (BassLookAndFeel::surface.withAlpha (0.4f));
            g.fillRect (row);
        }
    }

    // Column header labels
    auto hdr = getLocalBounds().reduced (8).removeFromTop (kHeaderH);
    g.setFont (juce::Font (UIConst::uiFontSize, juce::Font::bold));
    g.setColour (BassLookAndFeel::textDim);

    const int srcW = 150, gap = 6, tgtW = 170;
    hdr.removeFromLeft (24); // slot number column
    g.drawText ("SOURCE", hdr.removeFromLeft (srcW), juce::Justification::centred);
    hdr.removeFromLeft (gap);
    g.drawText ("TARGET", hdr.removeFromLeft (tgtW), juce::Justification::centred);
    hdr.removeFromLeft (gap);
    g.drawText ("AMOUNT", hdr, juce::Justification::centred);
}

void ModMatrixComponent::resized()
{
    auto inner = getLocalBounds().reduced (8);
    inner.removeFromTop (kHeaderH); // skip column header row

    const int numW = 24, srcW = 150, gap = 6, tgtW = 170;

    for (int i = 0; i < 8; ++i)
    {
        auto& row = rows_[(size_t) i];
        auto r = inner.removeFromTop (kRowH).reduced (2, 4);

        r.removeFromLeft (numW); // slot number space
        row.sourceCombo.setBounds (r.removeFromLeft (srcW));
        r.removeFromLeft (gap);
        row.targetCombo.setBounds (r.removeFromLeft (tgtW));
        r.removeFromLeft (gap);
        row.amountSlider.setBounds (r);
    }
}
