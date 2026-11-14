#include "MainViewComponent.h"
#include "UIConstants.h"

MainViewComponent::MainViewComponent (JQGunkAudioProcessor& p)
    : processor (p),
      gateSection     (p.apvts),
      envelopeSection (p.apvts),
      filterSection   (p, p.apvts),
      combinedOscSection (p.apvts,
          OscParamIds { ParamIDs::waveform,    ParamIDs::oscLevel,  ParamIDs::unisonVoices,    ParamIDs::unisonDetune,    ParamIDs::unisonBlend,    ParamIDs::octaveShift,    ParamIDs::morph,    ParamIDs::coarseTune,   ParamIDs::fineTune,     ParamIDs::unisonWidth     },
          OscParamIds { ParamIDs::osc2Waveform, ParamIDs::osc2Level, ParamIDs::osc2UnisonVoices, ParamIDs::osc2UnisonDetune, ParamIDs::osc2UnisonBlend, ParamIDs::osc2OctaveShift, ParamIDs::osc2Morph, ParamIDs::osc2CoarseTune, ParamIDs::osc2FineTune, ParamIDs::osc2UnisonWidth }),
      transientSection (p.apvts, JQGunkAudioProcessor::getTransientSamplesDir()),
      irSection     (p.apvts, JQGunkAudioProcessor::getIRPresetsDir()),
      driveSection  (p.apvts),
      outputSection (p.apvts),
      lfoSection    (p.apvts)
{
    // OSC 1 callbacks
    combinedOscSection.isCustomWavetableLoaded[0]   = [&p] { return p.isCustomWavetableLoaded(); };
    combinedOscSection.isCustomWaveformActive[0]    = [&p] { return p.isCustomWaveformActive(); };
    combinedOscSection.reactivateCustomWavetable[0] = [&p] { p.reactivateCustomWavetable(); };
    combinedOscSection.loadWavetableFromFile[0]     = [&p] (const juce::File& f) { return p.loadWavetableFromFile (f); };
    combinedOscSection.getNumFrames[0]              = [&p] { return p.getOscNumFrames(); };
    combinedOscSection.getDisplayFrame[0]           = [&p] (std::vector<float>& buf) { p.getOscFrameForDisplay (0, buf); };
    combinedOscSection.getWavetableName[0]          = [&p] { return p.getOscWavetableName (0); };

    // OSC 2 callbacks
    combinedOscSection.isCustomWavetableLoaded[1]   = [&p] { return p.isCustomWavetable2Loaded(); };
    combinedOscSection.isCustomWaveformActive[1]    = [&p] { return p.isCustomWaveform2Active(); };
    combinedOscSection.reactivateCustomWavetable[1] = [&p] { p.reactivateCustomWavetable2(); };
    combinedOscSection.loadWavetableFromFile[1]     = [&p] (const juce::File& f) { return p.loadWavetable2FromFile (f); };
    combinedOscSection.getNumFrames[1]              = [&p] { return p.getOsc2NumFrames(); };
    combinedOscSection.getDisplayFrame[1]           = [&p] (std::vector<float>& buf) { p.getOscFrameForDisplay (1, buf); };
    combinedOscSection.getWavetableName[1]          = [&p] { return p.getOscWavetableName (1); };

    combinedOscSection.getModDetune = [&p] (int idx) { return p.getModulatedDetune (idx); };
    combinedOscSection.getModBlend  = [&p] (int idx) { return p.getModulatedBlend  (idx); };
    combinedOscSection.getModWidth  = [&p] (int idx) { return p.getModulatedWidth  (idx); };

    transientSection.loadSampleFromFile = [&p] (const juce::File& f) { return p.loadTransientSampleFromFile (f); };
    transientSection.getLoadedPath      = [&p] { return p.getTransientSamplePath(); };
    irSection.loadIRFromFile = [&p] (const juce::File& f) { return p.loadIRFromFile (f); };
    irSection.getLoadedPath  = [&p] { return p.getIRSamplePath(); };

    for (auto* c : std::initializer_list<juce::Component*> {
             &gateSection, &envelopeSection, &filterSection, &combinedOscSection,
             &transientSection, &irSection, &driveSection, &outputSection, &lfoSection })
        addAndMakeVisible (c);
}

void MainViewComponent::tick (bool gateOpen, bool transientFired)
{
    combinedOscSection.updateButtonStates();
    combinedOscSection.repaintDisplays();
    transientSection.updateButtonStates();
    irSection.updateButtonStates();
    filterSection.repaintDisplay();

    for (auto* s : std::initializer_list<SectionPanel*> { &combinedOscSection, &transientSection,
                                                          &irSection, &driveSection, &filterSection })
        s->refreshDimmed();

    gateSection.setMeterValues (processor.getEnvelope(), gateOpen, transientFired);
    envelopeSection.pushSample (processor.getModEnvelope(), processor.getADSRVelocity());
    lfoSection.pushSample (processor.getLFOValue());
}

void MainViewComponent::resized()
{
    // Three rows in signal-flow order. Widths are proportions of each row.
    //   Tracking | Oscillators | Transient
    //   ADSR     | Drive       | Filter
    //   LFO      | IR          | Output
    auto area = getLocalBounds();
    const int g = UIConst::sectionGap;
    const int h = area.getHeight() - 2 * g;

    auto row1 = area.removeFromTop (h * 415 / 1000);
    area.removeFromTop (g);
    auto row2 = area.removeFromTop (h * 352 / 1000);
    area.removeFromTop (g);
    auto row3 = area;

    auto split = [g] (juce::Rectangle<int> row, std::initializer_list<std::pair<juce::Component*, int>> cols)
    {
        const int w = row.getWidth() - g * ((int) cols.size() - 1);
        int i = 0;
        for (auto [c, pct] : cols)
        {
            c->setBounds (++i == (int) cols.size() ? row : row.removeFromLeft (w * pct / 100));
            row.removeFromLeft (g);
        }
    };

    split (row1, { { &gateSection, 22 }, { &combinedOscSection, 52 }, { &transientSection, 26 } });
    split (row2, { { &envelopeSection, 34 }, { &driveSection, 20 }, { &filterSection, 46 } });
    split (row3, { { &lfoSection, 32 }, { &irSection, 26 }, { &outputSection, 42 } });
}
