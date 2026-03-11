#include "PluginProcessor.h"
#include "ModMatrix.h"
#include "PluginEditor.h"
#include "ParamFormatters.h"
#include "DebugLog.h"

using namespace ParamFormatters;

//==============================================================================
JQGunkAudioProcessor::JQGunkAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    modMatrix.init (apvts);
}

JQGunkAudioProcessor::~JQGunkAudioProcessor() {}

//==============================================================================
static void addOscillatorParams (juce::AudioProcessorValueTreeState::ParameterLayout& layout, int idx)
{
    // For idx=1: IDs are plain ("waveform", "unisonVoices", …)
    // For idx=2: IDs are "osc2"-prefixed ("osc2Waveform", "osc2UnisonVoices", …)
    auto pid = [idx] (const char* base) -> juce::String
    {
        if (idx == 1) return base;
        juce::String s (base);
        return "osc2" + s.substring (0, 1).toUpperCase() + s.substring (1);
    };
    auto pname = [idx] (const char* base) -> juce::String
    {
        return idx == 1 ? juce::String (base) : juce::String ("OSC 2 ") + base;
    };

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        pid (ParamIDs::waveform), pname ("Waveform"),
        juce::StringArray { "Triangle", "Square", "Sawtooth" }, 0));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        idx == 1 ? ParamIDs::oscLevel : ParamIDs::osc2Level,
        idx == 1 ? "OSC Level" : "OSC 2 Level",
        juce::NormalisableRange<float> (0.0f, 2.0f, 0.01f), 1.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        pctFmt(), pctParse (0.0f, 2.0f)));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        pid (ParamIDs::unisonVoices), pname ("Unison Voices"),
        juce::NormalisableRange<float> (1.0f, 8.0f, 1.0f), 1.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        [] (float v, int) -> juce::String { return juce::String (juce::roundToInt (v)); },
        [] (const juce::String& t) -> float
            { return juce::jlimit (1.0f, 8.0f, (float) juce::roundToInt (t.getFloatValue())); }));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        pid (ParamIDs::unisonDetune), pname ("Unison Detune"),
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 20.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        [] (float v, int) -> juce::String { return juce::String (v, 1) + " ct"; },
        [] (const juce::String& t) -> float
            { return juce::jlimit (0.0f, 100.0f, t.retainCharacters ("0123456789.").getFloatValue()); }));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        pid (ParamIDs::unisonBlend), pname ("Unison Blend"),
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        pctFmt(), pctParse (0.0f, 1.0f)));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        pid (ParamIDs::octaveShift), pname ("Octave Shift"),
        juce::StringArray { "-2", "-1", "0", "+1", "+2" }, 2));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        pid (ParamIDs::coarseTune), pname ("Coarse Tune"),
        juce::NormalisableRange<float> (-24.0f, 24.0f, 1.0f), 0.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        semitoneFmt(), semitoneParse (-24.0f, 24.0f)));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        pid (ParamIDs::fineTune), pname ("Fine Tune"),
        juce::NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 0.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        [] (float v, int) -> juce::String
        {
            if (v >= 0.0f) return "+" + juce::String (v, 1) + " ct";
            return juce::String (v, 1) + " ct";
        },
        [] (const juce::String& t) -> float
            { return juce::jlimit (-100.0f, 100.0f, t.retainCharacters ("0123456789.-").getFloatValue()); }));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        pid (ParamIDs::morph), pname ("Morph"),
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        pctFmt(), pctParse (0.0f, 1.0f)));
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
JQGunkAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        ParamIDs::pitchWindow, "Pitch Window",
        juce::StringArray { "4096 (B0)", "2048 (fast)" }, 0));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "gateThreshold", "Gate Threshold",
        logAmpRange (0.001f, 0.04f),
        0.003162f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        logAmpFmt(), logAmpParse (0.001f, 0.04f)));

    // Hysteresis is stored in dB; the gate opens at thresh * 10^(hyst/20)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "gateHysteresis", "Gate Hysteresis",
        juce::NormalisableRange<float> (0.0f, 6.0f, 0.1f), 3.5f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        [] (float v, int) -> juce::String { return "+" + juce::String (v, 1) + " dB"; },
        [] (const juce::String& t) -> float
            { return juce::jlimit (0.0f, 6.0f, t.retainCharacters ("0123456789.").getFloatValue()); }));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "transientSens", "Transient Sens",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        pctFmt(), pctParse (0.0f, 1.0f)));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "glide", "Glide",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        timeFmt(), timeParse (0.0f, 1.0f)));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "dryLevel", "Dry Level",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        pctFmt(), pctParse (0.0f, 1.0f)));

    addOscillatorParams (layout, 1);

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "subLevel", "Sub Level",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        pctFmt(), pctParse (0.0f, 1.0f)));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        "subOctave", "Sub Octave",
        juce::StringArray { "-2", "-1", "0", "+1" }, 1)); // default: -1

    layout.add (std::make_unique<juce::AudioParameterBool> (
        "subBypassFilter", "Sub Bypass Filter", true));

    addOscillatorParams (layout, 2);

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "envResonance", "Env Resonance",
        juce::NormalisableRange<float> (0.0f, 8.0f, 0.01f), 2.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        scaledPctFmt (8.0f), scaledPctParse (0.0f, 8.0f)));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        "filterType", "Filter Type",
        juce::StringArray { "LP", "HP", "BP" },
        0)); // default: LP

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "freqTracking", "Freq Tracking",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 1.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        pctFmt(), pctParse (0.0f, 1.0f)));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "filterFreq", "Filter Freq",
        juce::NormalisableRange<float> (-2000.0f, 4000.0f), 550.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        [] (float v, int) -> juce::String { return juce::String (juce::roundToInt (v)) + " Hz"; },
        [] (const juce::String& t) -> float
            { return juce::jlimit (-2000.0f, 4000.0f, t.retainCharacters ("-0123456789.").getFloatValue()); }));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "masterVolume", "Master Volume",
        juce::NormalisableRange<float> (0.0f, 2.0f, 0.01f), 1.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        pctFmt(), pctParse (0.0f, 2.0f)));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        "ampEnvSource", "Amp Env Source",
        juce::StringArray { "Source", "Env Module" }, 0));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "lfoRate", "LFO Rate",
        juce::NormalisableRange<float> (0.01f, 20.0f, 0.0f, 0.3f), 1.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        [] (float v, int) -> juce::String { return juce::String (v, 2) + " Hz"; },
        [] (const juce::String& t) -> float
            { return juce::jlimit (0.01f, 20.0f, t.retainCharacters ("0123456789.").getFloatValue()); }));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        "lfoShape", "LFO Shape",
        juce::StringArray { "Sine", "Tri", "Square", "Saw" }, 0));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "lfoAmount", "LFO Amount",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 1.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        pctFmt(), pctParse (0.0f, 1.0f)));

    ModMatrix::addParameters (layout);

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "transientPitch", "Transient Pitch",
        juce::NormalisableRange<float> (-48.0f, 48.0f, 0.1f), 0.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        semitoneFmt(), semitoneParse (-48.0f, 48.0f)));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "transientLevel", "Transient Level",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        pctFmt(), pctParse (0.0f, 1.0f)));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "transientAttack", "Transient Attack",
        juce::NormalisableRange<float> (0.0001f, 0.01f, 0.0001f, 0.3f), 0.005f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        timeFmt(), timeParse (0.0001f, 0.01f)));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "transientHold", "Transient Hold",
        juce::NormalisableRange<float> (0.0f, 0.5f, 0.0001f, 0.4f), 0.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        timeFmt(), timeParse (0.0f, 0.5f)));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "transientDecay", "Transient Decay",
        juce::NormalisableRange<float> (0.0001f, 0.2f, 0.0001f, 0.4f), 0.1f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        timeFmt(), timeParse (0.0001f, 0.05f)));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "transientDry", "Transient Dry",
        juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        pctFmt(), pctParse (0.0f, 2.0f)));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "modEnvAttack", "Env Attack",
        juce::NormalisableRange<float> (0.001f, 2.0f, 0.0f, 0.3f), 0.01f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        timeFmt(), timeParse (0.001f, 2.0f)));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "modEnvDecay", "Env Decay",
        juce::NormalisableRange<float> (0.001f, 2.0f, 0.0f, 0.3f), 0.1f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        timeFmt(), timeParse (0.001f, 2.0f)));

    return layout;
}

//==============================================================================
void JQGunkAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    dbgLog ("prepareToPlay called | sr=" + juce::String (sampleRate)
            + " | osc=" + juce::String ((int) oscillator.getCurrentWaveform())
            + " | waveParam=" + juce::String ((int) apvts.getRawParameterValue (ParamIDs::waveform)->load())
            + " | paramWhenCustomLoaded=" + juce::String (paramWhenCustomLoaded)
            + " | customPath=" + customWavetablePath);

    currentSampleRate = sampleRate;
    pitchThread.prepare (sampleRate);
    scratch.prepare (samplesPerBlock);
    oscillator.reset();
    oscillator.setFrequency (0.0, sampleRate);   // primes currentSampleRate in oscillator
    subOscillator.reset();
    subOscillator.setFrequency (0.0, sampleRate); // primes currentSampleRate in SineOscillator
    osc2.reset();
    osc2.setFrequency (0.0, sampleRate);          // primes currentSampleRate in osc2
    {
        using namespace cycfi::q::literals;
        qEnvFollower = std::make_unique<cycfi::q::ar_envelope_follower>(2_ms, 100_ms, (float)sampleRate);
        qGate        = std::make_unique<cycfi::q::noise_gate>(-30_dB);
    }
    envelope = 0.0f;
    modEnvelope = 0.0f;
    glide.reset();
    gateIsOpen = false;
    envelopeFilter.reset();
    envelopeFilter.prepare (sampleRate);
    transientPlayer.prepare (sampleRate);
    onsetDetector.prepare (sampleRate);

    dbgLog ("prepareToPlay finished | osc=" + juce::String ((int) oscillator.getCurrentWaveform()));
}

void JQGunkAudioProcessor::releaseResources()
{

}

bool JQGunkAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

//==============================================================================
void JQGunkAudioProcessor::updateOscParams (const OscUpdateConfig& cfg)
{
    // --- Waveform ---
    const int waveIdx = (int) apvts.getRawParameterValue (cfg.waveformId)->load();
    const auto requested = static_cast<WaveformType> (waveIdx + 1);
    const bool customActive = (cfg.osc.getCurrentWaveform() == WaveformType::Custom);
    if (!customActive || waveIdx != cfg.paramWhenCustomLoaded)
    {
        if (cfg.osc.getCurrentWaveform() != requested)
            cfg.osc.setWaveform (requested);
    }

    // --- Unison ---
    const int   numVoices    = juce::roundToInt (apvts.getRawParameterValue (cfg.voicesId)->load());
    const float detuneOffset = modMatrix.getOffset (cfg.detuneTarget);
    const float blendOffset  = modMatrix.getOffset (cfg.blendTarget);
    cfg.lastDetuneOffset.store (detuneOffset, std::memory_order_relaxed);
    cfg.lastBlendOffset .store (blendOffset,  std::memory_order_relaxed);
    const float detuneCents = juce::jlimit (0.0f, 100.0f,
        apvts.getRawParameterValue (cfg.detuneId)->load() + detuneOffset);
    const float uniBlend = juce::jlimit (0.0f, 1.0f,
        apvts.getRawParameterValue (cfg.blendId)->load() + blendOffset);
    cfg.osc.setUnisonParams (numVoices, detuneCents, uniBlend);

    // --- Morph ---
    {
        const float morphBase = apvts.getRawParameterValue (cfg.morphId)->load();
        const float modulated = juce::jlimit (0.0f, 1.0f,
            morphBase + modMatrix.getOffset (cfg.morphTarget));
        cfg.lastMorphModulated.store (modulated, std::memory_order_relaxed);
        cfg.osc.setMorph (modulated);
    }
}

//==============================================================================
JQGunkAudioProcessor::BlockParams JQGunkAudioProcessor::readBlockParams() const
{
    BlockParams p;

    p.oscLevel = apvts.getRawParameterValue (ParamIDs::oscLevel)->load();
    p.dryLevel = apvts.getRawParameterValue (ParamIDs::dryLevel)->load();

    p.gateThresh = apvts.getRawParameterValue (ParamIDs::gateThreshold)->load();
    const float gateHyst = apvts.getRawParameterValue (ParamIDs::gateHysteresis)->load();
    p.openThresh = p.gateThresh * std::pow (10.0f, gateHyst / 20.0f);

    p.subLevel        = apvts.getRawParameterValue (ParamIDs::subLevel)->load();
    p.subOctaveIdx    = (int) apvts.getRawParameterValue (ParamIDs::subOctave)->load();
    p.subBypassFilter = apvts.getRawParameterValue (ParamIDs::subBypassFilter)->load() > 0.5f;
    p.subOctaveMult   = (p.subOctaveIdx == 0) ? 0.25f
                      : (p.subOctaveIdx == 1) ? 0.5f
                      : (p.subOctaveIdx == 2) ? 1.0f
                      :                         2.0f;

    p.resonance    = apvts.getRawParameterValue (ParamIDs::envResonance)->load();
    p.filterType   = (int) apvts.getRawParameterValue (ParamIDs::filterType)->load();
    p.freqTracking = apvts.getRawParameterValue (ParamIDs::freqTracking)->load();
    p.filterFreq   = apvts.getRawParameterValue (ParamIDs::filterFreq)->load();

    p.glideTime    = apvts.getRawParameterValue (ParamIDs::glide)->load();
    p.glideSamples = (p.glideTime > 0.0f) ? (int) (currentSampleRate * p.glideTime) : 0;

    p.octaveShift = (int) apvts.getRawParameterValue (ParamIDs::octaveShift)->load();

    p.osc2Level       = apvts.getRawParameterValue (ParamIDs::osc2Level)->load();
    p.osc2OctaveShift = (int) apvts.getRawParameterValue (ParamIDs::osc2OctaveShift)->load();

    p.transientLevel  = apvts.getRawParameterValue (ParamIDs::transientLevel)->load();
    p.transientAttack = apvts.getRawParameterValue (ParamIDs::transientAttack)->load();
    p.transientHold   = apvts.getRawParameterValue (ParamIDs::transientHold)->load();
    p.transientDecay  = apvts.getRawParameterValue (ParamIDs::transientDecay)->load();
    p.transientPitch  = apvts.getRawParameterValue (ParamIDs::transientPitch)->load();
    p.transientDry    = apvts.getRawParameterValue (ParamIDs::transientDry)->load();

    {
        const float c = (float) juce::roundToInt (apvts.getRawParameterValue (ParamIDs::coarseTune)->load());
        const float f = apvts.getRawParameterValue (ParamIDs::fineTune)->load();
        p.osc1PitchMult = std::pow (2.0f, (c + f / 100.0f) / 12.0f);
    }
    {
        const float c = (float) juce::roundToInt (apvts.getRawParameterValue (ParamIDs::osc2CoarseTune)->load());
        const float f = apvts.getRawParameterValue (ParamIDs::osc2FineTune)->load();
        p.osc2PitchMult = std::pow (2.0f, (c + f / 100.0f) / 12.0f);
    }

    {
        const float modAttack = apvts.getRawParameterValue (ParamIDs::modEnvAttack)->load();
        const float modDecay  = apvts.getRawParameterValue (ParamIDs::modEnvDecay)->load();
        p.modAttCoef = 1.0f - std::exp (-1.0f / ((float) currentSampleRate * modAttack));
        p.modDecCoef = 1.0f - std::exp (-1.0f / ((float) currentSampleRate * modDecay));
    }

    // 0 = least sensitive (threshold 0.6), 1 = most sensitive (threshold 0.1)
    // "energy" method default is 0.3; knob at ~60% gives default sensitivity.
    p.transientThreshold = 0.6f - apvts.getRawParameterValue (ParamIDs::transientSens)->load() * 0.5f;

    return p;
}

//==============================================================================
void JQGunkAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

#ifdef ENABLE_DEBUG_LOG
    perfSetup.start();
#endif

    const int   N           = buffer.getNumSamples();
    const int   numChannels = buffer.getNumChannels();
    const float* inputData  = buffer.getReadPointer (0);

    // 1. Snapshot all block-rate parameters and their derived values
    BlockParams p = readBlockParams();

    const float lfoShape        = apvts.getRawParameterValue (ParamIDs::lfoShape)->load();
    const int   ampEnvSourceIdx = (int) apvts.getRawParameterValue (ParamIDs::ampEnvSource)->load();
    float       masterVolume    = apvts.getRawParameterValue (ParamIDs::masterVolume)->load();

    // 2. LFO: render per-sample block (uses rate/amount from previous block)
    lfo.renderBlock (scratch.lfoBuf, N, lfoShape, currentSampleRate);

#ifdef ENABLE_DEBUG_LOG
    perfModMatrix.start();
#endif
    // 3. Mod matrix snapshot using midpoint LFO and end-of-prev-block envelope
    modMatrix.snapshot (envelope, modEnvelope, glide.getLastDetectedFreq(), scratch.lfoBuf[N / 2]);

    // 4. Apply block-rate mod offsets for structural params
    p.oscLevel      = juce::jlimit (0.0f, 2.0f,  p.oscLevel  + modMatrix.getOffset (ModTarget::Osc1Level));
    p.osc2Level     = juce::jlimit (0.0f, 2.0f,  p.osc2Level + modMatrix.getOffset (ModTarget::Osc2Level));
    p.subLevel      = juce::jlimit (0.0f, 1.0f,  p.subLevel  + modMatrix.getOffset (ModTarget::SubLevel));
    // FilterFreq is now per-sample via computeTargetBlock — not added to p.filterFreq here
    p.resonance     = juce::jlimit (0.0f, 8.0f,  p.resonance + modMatrix.getOffset (ModTarget::FilterRes));
    p.glideTime     = juce::jlimit (0.0f, 1.0f,  p.glideTime + modMatrix.getOffset (ModTarget::Glide));
    p.glideSamples  = (p.glideTime > 0.0f) ? (int) (currentSampleRate * p.glideTime) : 0;
    masterVolume    = juce::jlimit (0.0f, 2.0f, masterVolume + modMatrix.getOffset (ModTarget::MasterVolume));
    p.osc1PitchMult *= std::pow (2.0f, modMatrix.getOffset (ModTarget::Osc1FineTune) / 1200.0f);
    p.osc2PitchMult *= std::pow (2.0f, modMatrix.getOffset (ModTarget::Osc2FineTune) / 1200.0f);

    // Update LFO rate/amount for the next block
    lfo.updateModulated (apvts.getRawParameterValue (ParamIDs::lfoRate)->load(),   modMatrix.getOffset (ModTarget::LfoRate),
                         apvts.getRawParameterValue (ParamIDs::lfoAmount)->load(), modMatrix.getOffset (ModTarget::LfoAmount));

    // 5. Update oscillator structural params (waveform, morph, unison)
    updateOscParams ({ oscillator, paramWhenCustomLoaded,
        ParamIDs::waveform, ParamIDs::unisonVoices, ParamIDs::unisonDetune, ParamIDs::unisonBlend, ParamIDs::morph,
        ModTarget::Unison1Detune, ModTarget::Unison1Blend, ModTarget::Morph1,
        lastModDetuneOffset, lastModBlendOffset, lastModulatedMorph });

    updateOscParams ({ osc2, param2WhenCustomLoaded,
        ParamIDs::osc2Waveform, ParamIDs::osc2UnisonVoices, ParamIDs::osc2UnisonDetune, ParamIDs::osc2UnisonBlend, ParamIDs::osc2Morph,
        ModTarget::Unison2Detune, ModTarget::Unison2Blend, ModTarget::Morph2,
        lastModDetune2Offset, lastModBlend2Offset, lastModulatedMorph2 });

    {
        const int windowIdx = (int) apvts.getRawParameterValue (ParamIDs::pitchWindow)->load();
        pitchThread.setPendingWindowSize (windowIdx == 0 ? 4096 : 2048);
    }

    if (qGate)
    {
        qGate->onset_threshold   (p.openThresh);
        qGate->release_threshold (p.gateThresh);
    }

    onsetDetector.setThreshold (p.transientThreshold);

#ifdef ENABLE_DEBUG_LOG
    perfSetup.stop();
    perfModMatrix.stop();
    perfLoop.start();
    perfControl.start();
#endif

    // octaveShift indices: 0=-2, 1=-1, 2=0, 3=+1, 4=+2
    static constexpr float kOctMult[]  = { 0.25f, 0.5f, 1.0f, 2.0f, 4.0f };
    static constexpr float kOct2Mult[] = { 0.25f, 0.5f, 1.0f, 2.0f, 4.0f };
    const float octMult    = kOctMult [juce::jlimit (0, 4, p.octaveShift)];
    const float osc2OctMult = kOct2Mult[juce::jlimit (0, 4, p.osc2OctaveShift)];

    // Read latest detected pitch from pitch thread (single value per block)
    const float rawFreq = pitchThread.getDetectedFrequency();

    // --- CONTROL PASS: envelope, gate, mod envelope, pitch, glide ---
    for (int i = 0; i < N; ++i)
    {
        const float inputSample = inputData[i];

        scratch.envelopeBuf[i] = qEnvFollower ? (*qEnvFollower)(std::abs (inputSample)) : 0.0f;
        gateIsOpen             = qGate        ? (*qGate)(scratch.envelopeBuf[i])        : false;
        scratch.modEnvBuf[i]   = updateModEnvelopeSample (scratch.envelopeBuf[i], p);

#ifdef ENABLE_DEBUG_LOG
        perfOnset.start();
#endif
        updateTransientDetection (p, inputSample);
#ifdef ENABLE_DEBUG_LOG
        perfOnset.stop();
#endif

        scratch.gatedInput[i] = gateIsOpen ? inputSample : 0.0f;

        scratch.glidedFreq[i] = glide.update (rawFreq * octMult * p.osc1PitchMult,
                                               p.glideSamples, gateIsOpen);
    }

#ifdef ENABLE_DEBUG_LOG
    perfControl.stop();
#endif

    // Update member vars from last sample for next block's snapshot
    envelope    = scratch.envelopeBuf[N - 1];
    // modEnvelope already updated in-place by updateModEnvelopeSample

    // --- DERIVE PER-OSC FREQ BUFFERS (tight loops, auto-vectorizable) ---
    for (int i = 0; i < N; ++i)
        scratch.osc2Freq[i] = scratch.glidedFreq[i] * osc2OctMult * p.osc2PitchMult;
    for (int i = 0; i < N; ++i)
        scratch.subFreq[i]  = scratch.glidedFreq[i] * p.subOctaveMult;

    // --- PER-SAMPLE PITCH BUFFER (normalized 0..1 for mod matrix) ---
    for (int i = 0; i < N; ++i)
        scratch.pitchNorm[i] = (scratch.glidedFreq[i] > 0.0f)
                               ? juce::jlimit (0.0f, 1.0f, (scratch.glidedFreq[i] - 40.0f) / 360.0f)
                               : 0.0f;

    // --- FILTER CUTOFF BLOCK ---
#ifdef ENABLE_DEBUG_LOG
    perfModMatrix.start();
#endif
    modMatrix.computeTargetBlock (ModTarget::FilterFreq,
                                   scratch.envelopeBuf, scratch.modEnvBuf,
                                   scratch.pitchNorm, scratch.lfoBuf,
                                   scratch.filterCutoff, N);
#ifdef ENABLE_DEBUG_LOG
    perfModMatrix.stop();
#endif
    for (int i = 0; i < N; ++i)
        scratch.filterCutoff[i] = juce::jlimit (20.0f, 4000.0f,
            p.filterFreq + scratch.glidedFreq[i] * p.freqTracking + scratch.filterCutoff[i]);

    // --- AUDIO GENERATORS ---
#ifdef ENABLE_DEBUG_LOG
    perfOsc1.start();
#endif
    oscillator.processBlock   (scratch.glidedFreq, scratch.osc1Out, N);
    juce::FloatVectorOperations::multiply (scratch.osc1Out, p.oscLevel, N);
#ifdef ENABLE_DEBUG_LOG
    perfOsc1.stop();
    perfOsc2.start();
#endif

    osc2.processBlock         (scratch.osc2Freq, scratch.osc2Out, N);
    juce::FloatVectorOperations::multiply (scratch.osc2Out, p.osc2Level, N);
#ifdef ENABLE_DEBUG_LOG
    perfOsc2.stop();
    perfSub.start();
#endif

    subOscillator.processBlock (scratch.subFreq, scratch.subOut, N);
    juce::FloatVectorOperations::multiply (scratch.subOut, p.subLevel, N);
#ifdef ENABLE_DEBUG_LOG
    perfSub.stop();
    perfTransient.start();
#endif

    {
        static constexpr float kTransientRefHz = 110.0f;
        const float midFreq      = scratch.glidedFreq[N / 2];
        const float freqTrackRate = (midFreq > 0.0f) ? (midFreq / kTransientRefHz) : 1.0f;
        const float transRate    = freqTrackRate * std::pow (2.0f, p.transientPitch / 12.0f);
        transientPlayer.processBlock (scratch.transientOut, scratch.transientEnv, N, transRate);
    }
#ifdef ENABLE_DEBUG_LOG
    perfTransient.stop();
#endif

    // --- PRE-FILTER MIX: reuse osc1Out as preFxMix ---
    juce::FloatVectorOperations::add (scratch.osc1Out, scratch.osc2Out, N);
    if (! p.subBypassFilter)
        juce::FloatVectorOperations::add (scratch.osc1Out, scratch.subOut, N);

    // --- FILTER ---
#ifdef ENABLE_DEBUG_LOG
    perfFilter.start();
#endif
    envelopeFilter.processBlock (scratch.osc1Out, scratch.filterCutoff, N,
                                  p.resonance, static_cast<FilterType> (p.filterType));
#ifdef ENABLE_DEBUG_LOG
    perfFilter.stop();
#endif

    // --- FINAL MIX ---
#ifdef ENABLE_DEBUG_LOG
    perfMix.start();
#endif
    for (int i = 0; i < N; ++i)
    {
        const float ampEnv   = (ampEnvSourceIdx == 0) ? scratch.envelopeBuf[i] : scratch.modEnvBuf[i];
        const float postFx   = p.subBypassFilter ? scratch.subOut[i] : 0.0f;
        const float oscWet   = (scratch.osc1Out[i] + postFx) * ampEnv;
        const float transDry = inputData[i] * scratch.transientEnv[i] * p.transientDry;
        const float out      = (inputData[i] * p.dryLevel + oscWet
                                + scratch.transientOut[i] * p.transientLevel + transDry)
                               * masterVolume;
        for (int ch = 0; ch < numChannels; ++ch)
            buffer.getWritePointer (ch)[i] = out;
    }

    // --- POST: push gated input to pitch detector ---
#ifdef ENABLE_DEBUG_LOG
    perfMix.stop();
#endif
    pitchThread.pushSamples (scratch.gatedInput, N);

#ifdef ENABLE_DEBUG_LOG
    perfLoop.stop();
    if (++perfBlockCount >= kPerfPrintInterval)
    {
        const double bufMs = (buffer.getNumSamples() / currentSampleRate) * 1000.0;
        dbgLog ("PERF buf=" + juce::String (bufMs, 2) + "ms"
                " | setup avg=" + juce::String (perfSetup.avgMs(), 3) + "ms"
                "  max="        + juce::String (perfSetup.maxMs(), 3) + "ms"
                " | loop  avg=" + juce::String (perfLoop.avgMs(),  3) + "ms"
                "  max="        + juce::String (perfLoop.maxMs(),  3) + "ms"
                " | onset avg=" + juce::String (perfOnset.avgMs(), 3) + "ms"
                "  max="        + juce::String (perfOnset.maxMs(), 3) + "ms"
                " | ctrl avg="  + juce::String (perfControl.avgMs(), 3) + "ms"
                "  max="        + juce::String (perfControl.maxMs(), 3) + "ms"
                " | osc1 avg="  + juce::String (perfOsc1.avgMs(), 3) + "ms"
                "  max="        + juce::String (perfOsc1.maxMs(), 3) + "ms"
                " | osc2 avg="  + juce::String (perfOsc2.avgMs(), 3) + "ms"
                "  max="        + juce::String (perfOsc2.maxMs(), 3) + "ms"
                " | sub avg="   + juce::String (perfSub.avgMs(), 3) + "ms"
                "  max="        + juce::String (perfSub.maxMs(), 3) + "ms"
                " | filt avg="  + juce::String (perfFilter.avgMs(), 3) + "ms"
                "  max="        + juce::String (perfFilter.maxMs(), 3) + "ms"
                " | trans avg=" + juce::String (perfTransient.avgMs(), 3) + "ms"
                "  max="        + juce::String (perfTransient.maxMs(), 3) + "ms"
                " | mod avg="   + juce::String (perfModMatrix.avgMs(), 3) + "ms"
                "  max="        + juce::String (perfModMatrix.maxMs(), 3) + "ms"
                " | mix avg="   + juce::String (perfMix.avgMs(), 3) + "ms"
                "  max="        + juce::String (perfMix.maxMs(), 3) + "ms");
        perfSetup.reset();
        perfLoop.reset();
        perfOnset.reset();
        perfControl.reset();
        perfOsc1.reset();
        perfOsc2.reset();
        perfSub.reset();
        perfFilter.reset();
        perfTransient.reset();
        perfModMatrix.reset();
        perfMix.reset();
        perfBlockCount = 0;
    }
#endif
}

float JQGunkAudioProcessor::updateModEnvelopeSample (float envSample, const BlockParams& p)
{
    if (envSample > modEnvelope)
        modEnvelope += p.modAttCoef * (envSample - modEnvelope);
    else
        modEnvelope += p.modDecCoef * (envSample - modEnvelope);
    return modEnvelope;
}

void JQGunkAudioProcessor::updateTransientDetection (const BlockParams& p, float inputSample)
{
    const float gated = gateIsOpen ? inputSample : 0.0f;
    if (onsetDetector.processSample (gated))
    {
        transientFlag.store (true, std::memory_order_relaxed);
        transientPlayer.trigger (p.transientAttack, p.transientHold, p.transientDecay);
    }
}


//==============================================================================
bool JQGunkAudioProcessor::isCustomWaveformActive() const
{
    return oscillator.getCurrentWaveform() == WaveformType::Custom;
}

void JQGunkAudioProcessor::reactivateCustomWavetable()
{
    if (customWavetablePath.isNotEmpty())
        loadWavetableFromFile (juce::File (customWavetablePath));
}

//==============================================================================
bool JQGunkAudioProcessor::isCustomWaveform2Active() const
{
    return osc2.getCurrentWaveform() == WaveformType::Custom;
}

void JQGunkAudioProcessor::reactivateCustomWavetable2()
{
    if (customWavetable2Path.isNotEmpty())
        loadWavetable2FromFile (juce::File (customWavetable2Path));
}

bool JQGunkAudioProcessor::loadTransientSampleFromFile (const juce::File& file)
{
    if (! transientPlayer.loadFromFile (file)) return false;
    transientSamplePath = file.getFullPathName();
    return true;
}

bool JQGunkAudioProcessor::isTransientSampleLoaded() const
{
    return transientPlayer.isSampleLoaded();
}

juce::String JQGunkAudioProcessor::getTransientSamplePath() const
{
    return transientSamplePath;
}

//==============================================================================
bool JQGunkAudioProcessor::loadWavetable2FromFile (const juce::File& file)
{
    const bool ok = file.hasFileExtension (".wt") ? osc2.loadWTFile (file)
                                                   : osc2.loadFromFile (file);
    if (! ok) return false;
    customWavetable2Path = file.getFullPathName();
    param2WhenCustomLoaded = (int) apvts.getRawParameterValue (ParamIDs::osc2Waveform)->load();
    return true;
}

//==============================================================================
bool JQGunkAudioProcessor::loadWavetableFromFile (const juce::File& file)
{
    const bool ok = file.hasFileExtension (".wt") ? oscillator.loadWTFile (file)
                                                   : oscillator.loadFromFile (file);
    if (! ok) return false;
    customWavetablePath = file.getFullPathName();
    paramWhenCustomLoaded = (int) apvts.getRawParameterValue (ParamIDs::waveform)->load();
    return true;
}

//==============================================================================
int JQGunkAudioProcessor::getNumPrograms()
{
    return (int) presetManager.getPresets().size();
}

int JQGunkAudioProcessor::getCurrentProgram()
{
    return presetManager.getCurrentIndex();
}

void JQGunkAudioProcessor::setCurrentProgram (int index)
{
    if (presetManager.loadPreset (index))
        syncOscillatorAfterPresetLoad();
}

const juce::String JQGunkAudioProcessor::getProgramName (int index)
{
    const auto& p = presetManager.getPresets();
    return (index >= 0 && (size_t) index < p.size()) ? p[(size_t) index].name : juce::String{};
}

void JQGunkAudioProcessor::syncOscillatorAfterPresetLoad()
{
    customWavetablePath = {};
    paramWhenCustomLoaded = -1;
    const int waveIdx = (int) apvts.getRawParameterValue (ParamIDs::waveform)->load();
    oscillator.setWaveform (static_cast<WaveformType> (waveIdx + 1));

    customWavetable2Path = {};
    param2WhenCustomLoaded = -1;
    const int wave2Idx = (int) apvts.getRawParameterValue (ParamIDs::osc2Waveform)->load();
    osc2.setWaveform (static_cast<WaveformType> (wave2Idx + 1));
}

//==============================================================================
juce::AudioProcessorEditor* JQGunkAudioProcessor::createEditor()
{
    return new JQGunkAudioProcessorEditor (*this);
}

void JQGunkAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    if (isCustomWaveformActive())
        xml->setAttribute ("customWavetablePath", customWavetablePath);
    else
        xml->removeAttribute ("customWavetablePath");
    if (isCustomWaveform2Active())
        xml->setAttribute ("customWavetablePath2", customWavetable2Path);
    else
        xml->removeAttribute ("customWavetablePath2");
    if (isTransientSampleLoaded())
        xml->setAttribute ("transientSamplePath", transientSamplePath);
    else
        xml->removeAttribute ("transientSamplePath");
    xml->setAttribute ("currentPresetIndex", presetManager.getCurrentIndex());
    copyXmlToBinary (*xml, dest);

    dbgLog ("getStateInformation | waveParam=" + juce::String ((int) apvts.getRawParameterValue (ParamIDs::waveform)->load())
            + " | osc=" + juce::String ((int) oscillator.getCurrentWaveform())
            + " | customActive=" + juce::String (isCustomWaveformActive() ? 1 : 0)
            + " | customPath=" + customWavetablePath
            + " | xml=" + xml->toString());
}

void JQGunkAudioProcessor::restoreOscWavetable (const juce::XmlElement* xml, int oscIdx)
{
    WavetableOscillator& oscRef  = (oscIdx == 1) ? oscillator : osc2;
    juce::String&        pathRef = (oscIdx == 1) ? customWavetablePath : customWavetable2Path;
    int&                 paramRef = (oscIdx == 1) ? paramWhenCustomLoaded : param2WhenCustomLoaded;
    const juce::String   attrName = (oscIdx == 1) ? "customWavetablePath" : "customWavetablePath2";
    const juce::String   waveId   = (oscIdx == 1) ? ParamIDs::waveform : ParamIDs::osc2Waveform;

    pathRef = xml->getStringAttribute (attrName, {});
    const int waveIdx = (int) apvts.getRawParameterValue (waveId)->load();

    if (pathRef.isNotEmpty())
    {
        juce::File f (pathRef);
        if (f.existsAsFile())
        {
            if (f.hasFileExtension (".wt")) oscRef.loadWTFile (f);
            else                            oscRef.loadFromFile (f);
            paramRef = waveIdx;
        }
        else
        {
            // File gone — restore standard waveform
            pathRef = {};
            oscRef.setWaveform (static_cast<WaveformType> (waveIdx + 1));
            paramRef = -1;
        }
    }
    else
    {
        oscRef.setWaveform (static_cast<WaveformType> (waveIdx + 1));
        paramRef = -1;
    }
}

void JQGunkAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));

    dbgLog ("setStateInformation called | xmlValid=" + juce::String (xml ? 1 : 0)
            + (xml ? " | tagMatch=" + juce::String (xml->hasTagName (apvts.state.getType()) ? 1 : 0)
                        + " | xml=" + xml->toString()
                   : ""));

    if (! xml || ! xml->hasTagName (apvts.state.getType()))
    {
        dbgLog ("setStateInformation: early return — bad XML");
        return;
    }

    apvts.replaceState (juce::ValueTree::fromXml (*xml));
    presetManager.setCurrentIndex (xml->getIntAttribute ("currentPresetIndex", -1));

    const int waveIdx = (int) apvts.getRawParameterValue (ParamIDs::waveform)->load();
    dbgLog ("setStateInformation: after replaceState | waveParam=" + juce::String (waveIdx));

    restoreOscWavetable (xml.get(), 1);
    restoreOscWavetable (xml.get(), 2);

    // Transient sample
    transientSamplePath = xml->getStringAttribute ("transientSamplePath", {});
    if (transientSamplePath.isNotEmpty())
    {
        juce::File f (transientSamplePath);
        if (f.existsAsFile())
            transientPlayer.loadFromFile (f);
        else
            transientSamplePath = {};
    }

    dbgLog ("setStateInformation: restored waveforms | waveIdx=" + juce::String (waveIdx)
            + " | osc=" + juce::String ((int) oscillator.getCurrentWaveform()));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new JQGunkAudioProcessor();
}
