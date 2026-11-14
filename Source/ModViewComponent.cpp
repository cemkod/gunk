#include "ModViewComponent.h"
#include "TopBarComponent.h"
#include "UIConstants.h"

//==============================================================================
void TargetComboBox::showPopup()
{
    juce::PopupMenu menu;
    menu.addItem ((int) ModTarget::None, "None");
    menu.addSeparator();

    juce::PopupMenu osc1;
    osc1.addItem ((int) ModTarget::Osc1Level,     "Level");
    osc1.addItem ((int) ModTarget::Morph1,        "Morph");
    osc1.addItem ((int) ModTarget::Osc1FineTune,  "Fine Tune");
    osc1.addItem ((int) ModTarget::Unison1Detune, "Unison Detune");
    osc1.addItem ((int) ModTarget::Unison1Blend,  "Unison Blend");
    osc1.addItem ((int) ModTarget::Width1,        "Unison Width");
    menu.addSubMenu ("OSC 1", osc1);

    juce::PopupMenu osc2;
    osc2.addItem ((int) ModTarget::Osc2Level,     "Level");
    osc2.addItem ((int) ModTarget::Morph2,        "Morph");
    osc2.addItem ((int) ModTarget::Osc2FineTune,  "Fine Tune");
    osc2.addItem ((int) ModTarget::Unison2Detune, "Unison Detune");
    osc2.addItem ((int) ModTarget::Unison2Blend,  "Unison Blend");
    osc2.addItem ((int) ModTarget::Width2,        "Unison Width");
    menu.addSubMenu ("OSC 2", osc2);

    juce::PopupMenu filter;
    filter.addItem ((int) ModTarget::FilterFreq, "Frequency");
    filter.addItem ((int) ModTarget::FilterRes,  "Resonance");
    filter.addItem ((int) ModTarget::Drive,      "Drive");
    menu.addSubMenu ("Filter", filter);

    juce::PopupMenu lfo;
    lfo.addItem ((int) ModTarget::LfoRate,   "Rate");
    lfo.addItem ((int) ModTarget::LfoAmount, "Amount");
    menu.addSubMenu ("LFO", lfo);

    juce::PopupMenu global;
    global.addItem ((int) ModTarget::Glide,        "Glide");
    global.addItem ((int) ModTarget::SubLevel,     "Sub Level");
    global.addItem ((int) ModTarget::MasterVolume, "Master Volume");
    menu.addSubMenu ("Global", global);

    // A PopupMenu only uses a LookAndFeel set on it; withTargetComponent (not a screen
    // area) makes it follow the editor's scale
    menu.setLookAndFeel (&getLookAndFeel());
    menu.showMenuAsync (
        juce::PopupMenu::Options().withTargetComponent (this),
        [safeThis = juce::Component::SafePointer<TargetComboBox> (this)] (int result)
        {
            if (safeThis == nullptr) return;
            if (result != 0)
                safeThis->setSelectedId (result, juce::sendNotification);
            safeThis->hidePopup();   // reset menuActive so the popup can be opened again
        });
}

//==============================================================================
void PitchTraceComponent::pushSample (float hz)
{
    history[(size_t) writePos] = hz;
    writePos = (writePos + 1) % (int) history.size();
    note = TopBarComponent::noteName (hz);
    repaint();
}

void PitchTraceComponent::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (BassLookAndFeel::surfaceDark);
    g.fillRoundedRectangle (b, UIConst::displayCornerRadius);
    g.setColour (BassLookAndFeel::border);
    g.drawRoundedRectangle (b.reduced (0.5f), UIConst::displayCornerRadius, 1.0f);

    // Current note in a box on the right
    auto noteBox = b.removeFromRight (juce::jmin (64.0f, b.getWidth() * 0.25f));
    g.setColour (BassLookAndFeel::border);
    g.fillRect (noteBox.getX(), noteBox.getY(), 1.0f, noteBox.getHeight());
    g.setColour (note == "--" ? BassLookAndFeel::textDim : BassLookAndFeel::accent);
    g.setFont (BassLookAndFeel::uiFont (UIConst::px (26.0f), true));
    g.drawText (note, noteBox, juce::Justification::centred);

    // Stepped trace, log 40..400 Hz; gaps where there is no pitch
    const auto area = b.reduced (6.0f, 8.0f);
    const int n = (int) history.size();
    auto toY = [&] (float hz)
    {
        const float t = std::log (juce::jlimit (40.0f, 400.0f, hz) / 40.0f) / std::log (10.0f);
        return area.getBottom() - t * area.getHeight();
    };
    juce::Path p;
    bool drawing = false;
    float lastY = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        const float hz = history[(size_t) ((writePos + i) % n)];
        const float x  = area.getX() + area.getWidth() * (float) i / (float) (n - 1);
        if (hz <= 0.0f) { drawing = false; continue; }
        const float y = toY (hz);
        if (! drawing) { p.startNewSubPath (x, y); drawing = true; }
        else           { p.lineTo (x, lastY); p.lineTo (x, y); }
        lastY = y;
    }
    g.setColour (lineColour);
    g.strokePath (p, juce::PathStrokeType (UIConst::displayStrokeWidth));
}

//==============================================================================
void ModSlotRow::AddButton::paintButton (juce::Graphics& g, bool over, bool down)
{
    const auto b = getLocalBounds().toFloat().reduced (1.0f);
    if (over || down)
    {
        g.setColour (BassLookAndFeel::buttonFill.withAlpha (down ? 0.8f : 0.5f));
        g.fillRoundedRectangle (b, UIConst::buttonCornerRadius);
    }
    juce::Path outline;
    outline.addRoundedRectangle (b, UIConst::buttonCornerRadius);
    juce::Path dashed;
    const float dashes[] = { 5.0f, 4.0f };
    juce::PathStrokeType (1.0f).createDashedStroke (dashed, outline, dashes, 2);
    g.setColour (over ? BassLookAndFeel::textDim : BassLookAndFeel::borderDim);
    g.fillPath (dashed);

    g.setColour (over ? BassLookAndFeel::text : BassLookAndFeel::textDim);
    g.setFont (BassLookAndFeel::uiFont (UIConst::px (15.0f), true));
    g.drawText ("+  Add modulation", getLocalBounds(), juce::Justification::centred);
}

ModSlotRow::ModSlotRow (juce::AudioProcessorValueTreeState& apvts, int slotIndex)
    : slot (slotIndex)
{
    const juce::String n (slotIndex);
    sourceParam = apvts.getParameter ("modSlot" + n + "Source");
    targetParam = apvts.getParameter ("modSlot" + n + "Target");
    amountParam = apvts.getParameter ("modSlot" + n + "Amount");
    jassert (sourceParam != nullptr && targetParam != nullptr && amountParam != nullptr);

    // Source: display names (Input Env, ADSR...) over the stored choice index
    for (int j = 0; j < ModMatrix::kSourceNames.size(); ++j)
        sourceCombo.addItem (BassLookAndFeel::modSourceNames[j], j + 1);
    sourceAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, "modSlot" + n + "Source", sourceCombo);
    addAndMakeVisible (sourceCombo);

    // Target: the item IDs are ModTarget values; the popup groups them by section
    for (int j = 0; j < ModMatrix::kTargetNames.size(); ++j)
        targetCombo.addItem (ModMatrix::kTargetNames[j], j + 1);
    targetAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, "modSlot" + n + "Target", targetCombo);
    addAndMakeVisible (targetCombo);

    amountSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    amountSlider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    amountSlider.setDoubleClickReturnValue (true, 0.0);
    BassLookAndFeel::setBarStyle (amountSlider);
    amountSlider.setTooltip ("Modulation amount (double-click resets to 0)");
    amountAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, "modSlot" + n + "Amount", amountSlider);
    amountSlider.onValueChange = [this]
    {
        const double v = amountSlider.getValue();
        amountValue.setText ((v >= 0.0 ? "+" : "") + juce::String (v, 2), juce::dontSendNotification);
    };
    amountSlider.onValueChange();
    addAndMakeVisible (amountSlider);

    amountValue.setFont (BassLookAndFeel::uiFont (UIConst::px (24.0f), true));
    amountValue.setColour (juce::Label::textColourId, BassLookAndFeel::accent);
    amountValue.setJustificationType (juce::Justification::centredRight);
    amountValue.setBorderSize ({});
    addAndMakeVisible (amountValue);

    clearBtn.setTooltip ("Clear this slot");
    clearBtn.onClick = [this] { clearSlot(); };
    addAndMakeVisible (clearBtn);

    addBtn.onClick = [this] { showAddMenu(); };
    addChildComponent (addBtn);

    sync();
}

void ModSlotRow::sync()
{
    const int src = juce::roundToInt (sourceParam->convertFrom0to1 (sourceParam->getValue()));
    const int tgt = juce::roundToInt (targetParam->convertFrom0to1 (targetParam->getValue()));

    if (src != lastSource)
    {
        lastSource = src;
        const auto col = BassLookAndFeel::modSourceColour (src);
        BassLookAndFeel::setComboDotColour (sourceCombo, src == 0 ? juce::Colours::transparentBlack : col);
        BassLookAndFeel::setFillColour (amountSlider, src == 0 ? BassLookAndFeel::iconDim : col);
    }

    setEmptyLook (src == 0 && tgt == 0);
}

void ModSlotRow::setEmptyLook (bool isEmpty)
{
    if (isEmpty == empty && addBtn.isVisible() == isEmpty) return;
    empty = isEmpty;
    for (auto* c : std::initializer_list<juce::Component*> { &sourceCombo, &targetCombo, &amountSlider, &amountValue, &clearBtn })
        c->setVisible (! empty);
    addBtn.setVisible (empty);
    repaint();
}

void ModSlotRow::showAddMenu()
{
    juce::PopupMenu menu;
    menu.addSectionHeader ("Source");
    for (int j = 1; j < BassLookAndFeel::modSourceNames.size(); ++j)
        menu.addItem (j, BassLookAndFeel::modSourceNames[j]);

    menu.setLookAndFeel (&getLookAndFeel());
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&addBtn),
        [safeThis = juce::Component::SafePointer<ModSlotRow> (this)] (int result)
        {
            if (safeThis == nullptr || result <= 0) return;
            setParamWithGesture (safeThis->sourceParam, (float) result);
            safeThis->sync();
        });
}

void ModSlotRow::setParamWithGesture (juce::RangedAudioParameter* p, float denormalisedValue)
{
    p->beginChangeGesture();
    p->setValueNotifyingHost (p->convertTo0to1 (denormalisedValue));
    p->endChangeGesture();
}

void ModSlotRow::clearSlot()
{
    setParamWithGesture (sourceParam, 0.0f);   // None
    setParamWithGesture (targetParam, 0.0f);   // None
    setParamWithGesture (amountParam, 0.0f);
    sync();
}

void ModSlotRow::paint (juce::Graphics& g)
{
    // Slot number box
    const auto nb = numberBox.toFloat().reduced (0.5f);
    g.setColour (BassLookAndFeel::surfaceDark);
    g.fillRoundedRectangle (nb, UIConst::buttonCornerRadius);
    g.setColour (BassLookAndFeel::border);
    g.drawRoundedRectangle (nb, UIConst::buttonCornerRadius, 1.0f);
    g.setColour (empty ? BassLookAndFeel::textDim : BassLookAndFeel::text);
    g.setFont (BassLookAndFeel::uiFont (UIConst::px (20.0f), true));
    g.drawText (juce::String (slot + 1), numberBox, juce::Justification::centred);

    // Arrow from source to target
    if (! empty)
    {
        const auto a = arrowArea.toFloat().withSizeKeepingCentre (20.0f, 10.0f);
        juce::Path arrow;
        arrow.startNewSubPath (a.getX(), a.getCentreY());
        arrow.lineTo (a.getRight(), a.getCentreY());
        arrow.startNewSubPath (a.getRight() - 6.0f, a.getY());
        arrow.lineTo (a.getRight(), a.getCentreY());
        arrow.lineTo (a.getRight() - 6.0f, a.getBottom());
        g.setColour (BassLookAndFeel::textDim);
        g.strokePath (arrow, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Separator under the row
    g.setColour (BassLookAndFeel::border.withAlpha (0.6f));
    g.fillRect (0, getHeight() - 1, getWidth(), 1);
}

void ModSlotRow::resized()
{
    auto r = getLocalBounds().reduced (0, 10);
    numberBox = r.removeFromLeft (40);
    r.removeFromLeft (16);
    addBtn.setBounds (r);

    clearBtn.setBounds (r.removeFromRight (40));
    r.removeFromRight (16);
    amountValue.setBounds (r.removeFromRight (78));
    r.removeFromRight (14);
    sourceCombo.setBounds (r.removeFromLeft (172));
    arrowArea = r.removeFromLeft (44);
    targetCombo.setBounds (r.removeFromLeft (196));
    r.removeFromLeft (18);
    amountSlider.setBounds (r);
}

//==============================================================================
ModViewComponent::ModViewComponent (JQGunkAudioProcessor& p)
    : processor (p)
{
    auto& apvts = p.apvts;
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using K  = BassLookAndFeel::KnobSize;

    inputTrace.setLineColour (BassLookAndFeel::modSourceColour (1));
    pitchTrace.setLineColour (BassLookAndFeel::modSourceColour (2));
    adsrTrace .setLineColour (BassLookAndFeel::modSourceColour (3));
    lfoTrace  .setLineColour (BassLookAndFeel::modSourceColour (4));

    inputCard.addAndMakeVisible (inputTrace);
    inputCard.onLayout = [this] (juce::Rectangle<int> body) { inputTrace.setBounds (body); };

    pitchCard.addAndMakeVisible (pitchTrace);
    pitchCard.onLayout = [this] (juce::Rectangle<int> body) { pitchTrace.setBounds (body); };

    // ADSR card: a second set of attachments to the mod envelope (D3), so the mod screen
    // is usable without switching back
    adsrCard.addAndMakeVisible (adsrTrace);
    BassLookAndFeel::setupRotarySlider (attackSlider,  attackLabel,  "ATTACK",  adsrCard);
    BassLookAndFeel::setupRotarySlider (decaySlider,   decayLabel,   "DECAY",   adsrCard);
    BassLookAndFeel::setupRotarySlider (sustainSlider, sustainLabel, "SUSTAIN", adsrCard);
    BassLookAndFeel::setupRotarySlider (releaseSlider, releaseLabel, "RELEASE", adsrCard);
    attackAtt  = std::make_unique<SA> (apvts, "modEnvAttack",  attackSlider);
    decayAtt   = std::make_unique<SA> (apvts, "modEnvDecay",   decaySlider);
    sustainAtt = std::make_unique<SA> (apvts, "modEnvSustain", sustainSlider);
    releaseAtt = std::make_unique<SA> (apvts, "modEnvRelease", releaseSlider);
    adsrCard.onLayout = [this] (juce::Rectangle<int> body)
    {
        BassLookAndFeel::placeKnobRow (body.removeFromBottom (BassLookAndFeel::knobCellHeight (K::small)),
                                       { { &attackSlider,  &attackLabel },
                                         { &decaySlider,   &decayLabel },
                                         { &sustainSlider, &sustainLabel },
                                         { &releaseSlider, &releaseLabel } }, 0, K::small);
        body.removeFromBottom (UIConst::knobGap);
        adsrTrace.setBounds (body);
    };

    // LFO card
    lfoCard.addAndMakeVisible (lfoTrace);
    BassLookAndFeel::setupRotarySlider (rateSlider,   rateLabel,   "RATE",   lfoCard);
    BassLookAndFeel::setupRotarySlider (amountSlider, amountLabel, "AMOUNT", lfoCard);
    BassLookAndFeel::setModTarget (rateSlider,   (int) ModTarget::LfoRate);
    BassLookAndFeel::setModTarget (amountSlider, (int) ModTarget::LfoAmount);
    rateAtt   = std::make_unique<SA> (apvts, "lfoRate",   rateSlider);
    amountAtt = std::make_unique<SA> (apvts, "lfoAmount", amountSlider);
    shapeCombo.addItemList ({ "Sine", "Triangle", "Square", "Saw" }, 1);
    shapeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, "lfoShape", shapeCombo);
    lfoCard.addAndMakeVisible (shapeCombo);
    BassLookAndFeel::setupCaption (shapeLabel, "SHAPE", lfoCard);
    lfoCard.onLayout = [this] (juce::Rectangle<int> body)
    {
        auto row = body.removeFromBottom (BassLookAndFeel::knobCellHeight (K::small));
        body.removeFromBottom (UIConst::knobGap);
        lfoTrace.setBounds (body);

        auto shape = row.removeFromRight (row.getWidth() * 42 / 100);
        shape = shape.withSizeKeepingCentre (shape.getWidth(), UIConst::captionH + 4 + UIConst::buttonH);
        shapeLabel.setBounds (shape.removeFromTop (UIConst::captionH));
        shape.removeFromTop (4);
        shapeCombo.setBounds (shape);
        BassLookAndFeel::placeKnobRow (row, { { &rateSlider,   &rateLabel },
                                              { &amountSlider, &amountLabel } }, 0, K::small);
    };

    for (auto* c : { &inputCard, &pitchCard, &adsrCard, &lfoCard })
        addAndMakeVisible (c);

    // Matrix
    for (int i = 0; i < (int) rows.size(); ++i)
    {
        rows[(size_t) i] = std::make_unique<ModSlotRow> (apvts, i);
        matrixPanel.addAndMakeVisible (*rows[(size_t) i]);
    }
    usedLabel.setFont (BassLookAndFeel::uiFont (UIConst::fontCaption, true));
    usedLabel.setColour (juce::Label::textColourId, BassLookAndFeel::textDim);
    usedLabel.setJustificationType (juce::Justification::centredRight);
    matrixPanel.addAndMakeVisible (usedLabel);
    matrixPanel.setHeaderRight (&usedLabel, 120);
    matrixPanel.onLayout = [this] (juce::Rectangle<int> body)
    {
        const int rowH = juce::jmin (66, body.getHeight() / (int) rows.size());
        for (auto& r : rows)
            r->setBounds (body.removeFromTop (rowH));
    };
    addAndMakeVisible (matrixPanel);
}

void ModViewComponent::tick()
{
    inputTrace.pushSample (processor.getEnvelope());
    pitchTrace.pushSample (processor.getDetectedFrequency());
    adsrTrace .pushSample (processor.getModEnvelope(), processor.getADSRVelocity());
    lfoTrace  .pushSample (processor.getLFOValue());

    int used = 0;
    for (auto& r : rows)
    {
        r->sync();
        used += r->isEmptySlot() ? 0 : 1;
    }
    usedLabel.setText (juce::String (used) + " / " + juce::String ((int) rows.size()) + " USED",
                       juce::dontSendNotification);
}

void ModViewComponent::resized()
{
    auto area = getLocalBounds();
    const int g = UIConst::sectionGap;

    auto left = area.removeFromLeft (area.getWidth() * 25 / 100);
    area.removeFromLeft (g);
    matrixPanel.setBounds (area);

    // Source cards, in source order
    const int h = left.getHeight() - 3 * g;
    inputCard.setBounds (left.removeFromTop (h * 19 / 100));  left.removeFromTop (g);
    pitchCard.setBounds (left.removeFromTop (h * 19 / 100));  left.removeFromTop (g);
    adsrCard .setBounds (left.removeFromTop (h * 33 / 100));  left.removeFromTop (g);
    lfoCard  .setBounds (left);
}
