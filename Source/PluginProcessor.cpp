#include "PluginProcessor.h"
#include "ModMatrix.h"
#include "PluginEditor.h"
#include "ParamFormatters.h"

using namespace ParamFormatters;

#ifdef ENABLE_DEBUG_LOG
static void dbgLog (const juce::String& msg)
{
    auto f = juce::File::getSpecialLocation (juce::File::userHomeDirectory)
                .getChildFile ("bass-synth-debug.log");
    f.appendText ("[" + juce::Time::getCurrentTime().toString (true, true, true, true) + "] "
                  + msg + "\n");
}
#else
static void dbgLog (const juce::String&) {}
#endif

//==============================================================================
JQGunkAudioProcessor::JQGunkAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

JQGunkAudioProcessor::~JQGunkAudioProcessor() {}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
JQGunkAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

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
        "transientSlope", "Transient Slope",
        juce::NormalisableRange<float> (0.0f, 5.0f, 0.1f, 0.4f), 3.1f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "glide", "Glide",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        timeFmt(), timeParse (0.0f, 1.0f)));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "oscLevel", "OSC Level",
        juce::NormalisableRange<float> (0.0f, 2.0f, 0.01f), 1.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        pctFmt(), pctParse (0.0f, 2.0f)));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "dryLevel", "Dry Level",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        pctFmt(), pctParse (0.0f, 1.0f)));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        "waveform", "Waveform",
        juce::StringArray { "Triangle", "Square", "Sawtooth" },
        0)); // default: Triangle

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
        "unisonVoices", "Unison Voices",
        juce::NormalisableRange<float> (1.0f, 8.0f, 1.0f), 1.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        [] (float v, int) -> juce::String { return juce::String (juce::roundToInt (v)); },
        [] (const juce::String& t) -> float
            { return juce::jlimit (1.0f, 8.0f, (float) juce::roundToInt (t.getFloatValue())); }));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "unisonDetune", "Unison Detune",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 20.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        [] (float v, int) -> juce::String { return juce::String (v, 1) + " ct"; },
        [] (const juce::String& t) -> float
            { return juce::jlimit (0.0f, 100.0f, t.retainCharacters ("0123456789.").getFloatValue()); }));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "subLevel", "Sub Level",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        pctFmt(), pctParse (0.0f, 1.0f)));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "unisonBlend", "Unison Blend",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        pctFmt(), pctParse (0.0f, 1.0f)));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        "octaveShift", "Octave Shift",
        juce::StringArray { "-2", "-1", "0", "+1", "+2" }, 2));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        "subOctave", "Sub Octave",
        juce::StringArray { "-2", "-1", "0", "+1" }, 1)); // default: -1

    layout.add (std::make_unique<juce::AudioParameterBool> (
        "subBypassFilter", "Sub Bypass Filter", true));

    // OSC 2 parameters (mirror of OSC 1)
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        "osc2Waveform", "OSC 2 Waveform",
        juce::StringArray { "Triangle", "Square", "Sawtooth" }, 0));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "osc2Level", "OSC 2 Level",
        juce::NormalisableRange<float> (0.0f, 2.0f, 0.01f), 1.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        pctFmt(), pctParse (0.0f, 2.0f)));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "osc2UnisonVoices", "OSC 2 Unison Voices",
        juce::NormalisableRange<float> (1.0f, 8.0f, 1.0f), 1.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        [] (float v, int) -> juce::String { return juce::String (juce::roundToInt (v)); },
        [] (const juce::String& t) -> float
            { return juce::jlimit (1.0f, 8.0f, (float) juce::roundToInt (t.getFloatValue())); }));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "osc2UnisonDetune", "OSC 2 Unison Detune",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 20.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        [] (float v, int) -> juce::String { return juce::String (v, 1) + " ct"; },
        [] (const juce::String& t) -> float
            { return juce::jlimit (0.0f, 100.0f, t.retainCharacters ("0123456789.").getFloatValue()); }));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "osc2UnisonBlend", "OSC 2 Unison Blend",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        pctFmt(), pctParse (0.0f, 1.0f)));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        "osc2OctaveShift", "OSC 2 Octave Shift",
        juce::StringArray { "-2", "-1", "0", "+1", "+2" }, 2));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "coarseTune", "Coarse Tune",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 1.0f), 0.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        semitoneFmt(), semitoneParse (-24.0f, 24.0f)));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "fineTune", "Fine Tune",
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
        "osc2CoarseTune", "OSC 2 Coarse Tune",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 1.0f), 0.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        semitoneFmt(), semitoneParse (-24.0f, 24.0f)));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "osc2FineTune", "OSC 2 Fine Tune",
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
        "morph", "Morph",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        pctFmt(), pctParse (0.0f, 1.0f)));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "osc2Morph", "OSC 2 Morph",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        pctFmt(), pctParse (0.0f, 1.0f)));

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
        "transientDecay", "Transient Decay",
        juce::NormalisableRange<float> (0.0001f, 0.05f, 0.0001f, 0.4f), 0.015f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        timeFmt(), timeParse (0.0001f, 0.05f)));

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
void JQGunkAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    dbgLog ("prepareToPlay called | sr=" + juce::String (sampleRate)
            + " | osc=" + juce::String ((int) oscillator.getCurrentWaveform())
            + " | waveParam=" + juce::String ((int) apvts.getRawParameterValue ("waveform")->load())
            + " | paramWhenCustomLoaded=" + juce::String (paramWhenCustomLoaded)
            + " | customPath=" + customWavetablePath);

    currentSampleRate = sampleRate;
    detector.reset();
    detector.setSampleRate (sampleRate);
    oscillator.reset();
    subOscillator.reset();
    osc2.reset();
    envelope = 0.0f;
    modEnvelope = 0.0f;
    glide.reset();
    gateIsOpen = false;
    envelopeFilter.reset();
    envelopeFilter.prepare (sampleRate);
    transientPlayer.prepare (sampleRate);

    pitchDetectorLPF.reset();
    *pitchDetectorLPF.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, 500.0f);

    dbgLog ("prepareToPlay finished | osc=" + juce::String ((int) oscillator.getCurrentWaveform()));
}

void JQGunkAudioProcessor::releaseResources() {}

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

    p.oscLevel = apvts.getRawParameterValue ("oscLevel")->load();
    p.dryLevel = apvts.getRawParameterValue ("dryLevel")->load();

    p.gateThresh = apvts.getRawParameterValue ("gateThreshold")->load();
    const float gateHyst = apvts.getRawParameterValue ("gateHysteresis")->load();
    p.openThresh = p.gateThresh * std::pow (10.0f, gateHyst / 20.0f);

    p.attackCoeff  = 1.0f - std::exp (-1.0f / (float) (currentSampleRate * kEnvAttack));
    p.releaseCoeff = 1.0f - std::exp (-1.0f / (float) (currentSampleRate * kEnvRelease));

    p.subLevel        = apvts.getRawParameterValue ("subLevel")->load();
    p.subOctaveIdx    = (int) apvts.getRawParameterValue ("subOctave")->load();
    p.subBypassFilter = apvts.getRawParameterValue ("subBypassFilter")->load() > 0.5f;
    p.subOctaveMult   = (p.subOctaveIdx == 0) ? 0.25f
                      : (p.subOctaveIdx == 1) ? 0.5f
                      : (p.subOctaveIdx == 2) ? 1.0f
                      :                         2.0f;

    p.resonance    = apvts.getRawParameterValue ("envResonance")->load();
    p.filterType   = (int) apvts.getRawParameterValue ("filterType")->load();
    p.freqTracking = apvts.getRawParameterValue ("freqTracking")->load();
    p.filterFreq   = apvts.getRawParameterValue ("filterFreq")->load();

    p.glideTime    = apvts.getRawParameterValue ("glide")->load();
    p.glideSamples = (p.glideTime > 0.0f) ? (int) (currentSampleRate * p.glideTime) : 0;

    p.octaveShift = (int) apvts.getRawParameterValue ("octaveShift")->load();

    p.osc2Level       = apvts.getRawParameterValue ("osc2Level")->load();
    p.osc2OctaveShift = (int) apvts.getRawParameterValue ("osc2OctaveShift")->load();

    p.transientLevel  = apvts.getRawParameterValue ("transientLevel")->load();
    p.transientAttack = apvts.getRawParameterValue ("transientAttack")->load();
    p.transientDecay  = apvts.getRawParameterValue ("transientDecay")->load();
    p.transientPitch  = apvts.getRawParameterValue ("transientPitch")->load();

    {
        const float c = (float) juce::roundToInt (apvts.getRawParameterValue ("coarseTune")->load());
        const float f = apvts.getRawParameterValue ("fineTune")->load();
        p.osc1PitchMult = std::pow (2.0f, (c + f / 100.0f) / 12.0f);
    }
    {
        const float c = (float) juce::roundToInt (apvts.getRawParameterValue ("osc2CoarseTune")->load());
        const float f = apvts.getRawParameterValue ("osc2FineTune")->load();
        p.osc2PitchMult = std::pow (2.0f, (c + f / 100.0f) / 12.0f);
    }

    return p;
}

//==============================================================================
void JQGunkAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    // 1. Snapshot all block-rate parameters and their derived values
    BlockParams p = readBlockParams();

    const float modAttack  = apvts.getRawParameterValue ("modEnvAttack")->load();
    const float modDecay   = apvts.getRawParameterValue ("modEnvDecay")->load();
    const float modAttCoef = 1.0f - std::exp (-1.0f / ((float) currentSampleRate * modAttack));
    const float modDecCoef = 1.0f - std::exp (-1.0f / ((float) currentSampleRate * modDecay));

    // LFO computation (once per block)
    const float lfoShape  = apvts.getRawParameterValue ("lfoShape")->load();
    const float lfoScaled = lfo.tick (lfoShape, buffer.getNumSamples(), currentSampleRate);

    const int ampEnvSourceIdx = (int) apvts.getRawParameterValue ("ampEnvSource")->load();
    float masterVolume  = apvts.getRawParameterValue ("masterVolume")->load();

    modMatrix.snapshot (apvts, envelope, modEnvelope, glide.getLastDetectedFreq(), lfoScaled);

    p.oscLevel   = juce::jlimit (0.0f, 2.0f,        p.oscLevel   + modMatrix.getOffset (ModTarget::Osc1Level));
    p.osc2Level  = juce::jlimit (0.0f, 2.0f,        p.osc2Level  + modMatrix.getOffset (ModTarget::Osc2Level));
    p.subLevel   = juce::jlimit (0.0f, 1.0f,        p.subLevel   + modMatrix.getOffset (ModTarget::SubLevel));
    p.filterFreq = juce::jlimit (-2000.0f, 4000.0f, p.filterFreq + modMatrix.getOffset (ModTarget::FilterFreq));
    p.resonance  = juce::jlimit (0.0f, 8.0f,        p.resonance  + modMatrix.getOffset (ModTarget::FilterRes));

    // Glide mod
    p.glideTime   = juce::jlimit (0.0f, 1.0f, p.glideTime + modMatrix.getOffset (ModTarget::Glide));
    p.glideSamples = (p.glideTime > 0.0f) ? (int) (currentSampleRate * p.glideTime) : 0;

    // LFO Rate/Amount mods: persist for next block so mod targets take effect
    lfo.updateModulated (apvts.getRawParameterValue ("lfoRate")->load(),   modMatrix.getOffset (ModTarget::LfoRate),
                         apvts.getRawParameterValue ("lfoAmount")->load(), modMatrix.getOffset (ModTarget::LfoAmount));

    // Master Volume mod
    masterVolume = juce::jlimit (0.0f, 2.0f, masterVolume + modMatrix.getOffset (ModTarget::MasterVolume));

    // Fine tune modulation (offset in cents → multiply pitch multiplier)
    p.osc1PitchMult *= std::pow (2.0f, modMatrix.getOffset (ModTarget::Osc1FineTune) / 1200.0f);
    p.osc2PitchMult *= std::pow (2.0f, modMatrix.getOffset (ModTarget::Osc2FineTune) / 1200.0f);

    updateOscParams ({ oscillator, paramWhenCustomLoaded,
        "waveform", "unisonVoices", "unisonDetune", "unisonBlend", "morph",
        ModTarget::Unison1Detune, ModTarget::Unison1Blend, ModTarget::Morph1,
        lastModDetuneOffset, lastModBlendOffset, lastModulatedMorph });

    updateOscParams ({ osc2, param2WhenCustomLoaded,
        "osc2Waveform", "osc2UnisonVoices", "osc2UnisonDetune", "osc2UnisonBlend", "osc2Morph",
        ModTarget::Unison2Detune, ModTarget::Unison2Blend, ModTarget::Morph2,
        lastModDetune2Offset, lastModBlend2Offset, lastModulatedMorph2 });
    envelopeFilter.prepare (currentSampleRate);

    const int numChannels  = buffer.getNumChannels();
    const int numSamples   = buffer.getNumSamples();
    const float* inputData = buffer.getReadPointer (0);

    for (int i = 0; i < numSamples; ++i)
    {
        const float inputSample = inputData[i];
        const float absSample   = std::abs (inputSample);

        // 2a. Gate follower (Schmitt trigger)
        if (absSample > envelope)
            envelope += p.attackCoeff  * (absSample - envelope);
        else
            envelope += p.releaseCoeff * (absSample - envelope);

        if (!gateIsOpen && envelope >= p.openThresh)
            gateIsOpen = true;
        else if (gateIsOpen && envelope < p.gateThresh)
            gateIsOpen = false;

        // Slew-limited mod envelope (independent ATK/DCY from gate envelope)
        if (envelope > modEnvelope)
            modEnvelope += modAttCoef * (envelope - modEnvelope);
        else
            modEnvelope += modDecCoef * (envelope - modEnvelope);

        // Transient detection (slope over lookback window)
        const float kSlopeThreshold = apvts.getRawParameterValue ("transientSlope")->load() * 5e-4f;
        
        const float oldEnvelope = envHistory[(size_t) envHistoryPos];
        envHistoryPos = (envHistoryPos + 1) % kSlopeLookback;
        envHistory[(size_t) envHistoryPos] = envelope;
        const float delta = envelope - oldEnvelope;
        if (delta > kSlopeThreshold && transientCooldown ==0)
        {
            transientFlag.store (true, std::memory_order_relaxed);
            transientCooldown = static_cast<int> (currentSampleRate * 0.100f);
            transientPlayer.trigger (p.transientAttack, p.transientDecay);
        }
        if (transientCooldown > 0) --transientCooldown;

        if (!gateIsOpen)
            detector.clearHistory();

        // 2b. Pitch detection
        float filteredForPitch = pitchDetectorLPF.processSample (inputSample);
        float detectedFreq     = detector.processSample (filteredForPitch, currentSampleRate);

        // octaveShift indices: 0=-2, 1=-1, 2=0, 3=+1, 4=+2
        static constexpr float kOctMult[] = { 0.25f, 0.5f, 1.0f, 2.0f, 4.0f };
        detectedFreq *= kOctMult[juce::jlimit (0, 4, p.octaveShift)];

        detectedFreq *= p.osc1PitchMult;

        // 2c. Glide (frequency ramp)
        const float glidedFreq = glide.update (detectedFreq, p.glideSamples, gateIsOpen);

        // 2d. Set oscillator frequencies
        if (glidedFreq > 0.0f)
        {
            oscillator.setFrequency    (glidedFreq,                  currentSampleRate);
            subOscillator.setFrequency (glidedFreq * p.subOctaveMult, currentSampleRate);
            static constexpr float kOct2Mult[] = { 0.25f, 0.5f, 1.0f, 2.0f, 4.0f };
            const float freq2 = glidedFreq
                              * kOct2Mult[juce::jlimit (0, 4, p.osc2OctaveShift)]
                              * p.osc2PitchMult;
            osc2.setFrequency (freq2, currentSampleRate);
        }
        else
        {
            oscillator.reset();
            subOscillator.reset();
            osc2.reset();
        }

        // 2e. Oscillator sample generation
        const float sawSample  = oscillator.getNextSample();
        const float subSample  = subOscillator.getNextSample();
        const float osc2Sample = osc2.getNextSample();

        // 2f. Filter routing
        // subBypassFilter=true: sub added after filter (default, bypasses auto-wah)
        // subBypassFilter=false: sub mixed into filter input
        const float osc1And2 = sawSample * p.oscLevel + osc2Sample * p.osc2Level;
        const float filterInput = p.subBypassFilter ? osc1And2 : (osc1And2 + subSample * p.subLevel);
        const float filteredSample = envelopeFilter.processSample (
            filterInput, glide.getLastDetectedFreq(),
            p.filterFreq, p.freqTracking, p.resonance,
            static_cast<FilterType> (p.filterType));

        // 2g. Output mix
        const float ampEnv = (ampEnvSourceIdx == 0) ? envelope : modEnvelope;
        const float oscWet    = filteredSample * ampEnv;
        const float subWet    = p.subBypassFilter ? subSample * p.subLevel * ampEnv : 0.0f;
        // Frequency tracking: scale playback rate so the transient follows the
        // detected pitch (referenced to 110 Hz = A2).  The pitch knob offsets
        // in semitones on top of that.
        static constexpr float kTransientRefHz = 110.0f;
        const float freqTrackRate = (glidedFreq > 0.0f) ? (glidedFreq / kTransientRefHz) : 1.0f;
        const float transRate = freqTrackRate * std::pow (2.0f, p.transientPitch / 12.0f);
        const float transSample = transientPlayer.getNextSample (transRate) * p.transientLevel;
        const float out       = inputSample * p.dryLevel + oscWet + subWet + transSample;

        for (int ch = 0; ch < numChannels; ++ch)
            buffer.getWritePointer (ch)[i] = out * masterVolume;
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
    param2WhenCustomLoaded = (int) apvts.getRawParameterValue ("osc2Waveform")->load();
    return true;
}

//==============================================================================
bool JQGunkAudioProcessor::loadWavetableFromFile (const juce::File& file)
{
    const bool ok = file.hasFileExtension (".wt") ? oscillator.loadWTFile (file)
                                                   : oscillator.loadFromFile (file);
    if (! ok) return false;
    customWavetablePath = file.getFullPathName();
    paramWhenCustomLoaded = (int) apvts.getRawParameterValue ("waveform")->load();
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
    const int waveIdx = (int) apvts.getRawParameterValue ("waveform")->load();
    oscillator.setWaveform (static_cast<WaveformType> (waveIdx + 1));

    customWavetable2Path = {};
    param2WhenCustomLoaded = -1;
    const int wave2Idx = (int) apvts.getRawParameterValue ("osc2Waveform")->load();
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

    dbgLog ("getStateInformation | waveParam=" + juce::String ((int) apvts.getRawParameterValue ("waveform")->load())
            + " | osc=" + juce::String ((int) oscillator.getCurrentWaveform())
            + " | customActive=" + juce::String (isCustomWaveformActive() ? 1 : 0)
            + " | customPath=" + customWavetablePath
            + " | xml=" + xml->toString());
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

    const int waveIdx = (int) apvts.getRawParameterValue ("waveform")->load();
    dbgLog ("setStateInformation: after replaceState | waveParam=" + juce::String (waveIdx));

    customWavetablePath = xml->getStringAttribute ("customWavetablePath", {});
    dbgLog ("setStateInformation: customWavetablePath=" + customWavetablePath);

    if (customWavetablePath.isNotEmpty())
    {
        juce::File f (customWavetablePath);
        const bool exists = f.existsAsFile();
        dbgLog ("setStateInformation: custom path exists=" + juce::String (exists ? 1 : 0));
        if (exists)
        {
            if (f.hasFileExtension (".wt")) oscillator.loadWTFile (f);
            else                            oscillator.loadFromFile (f);
            paramWhenCustomLoaded = waveIdx;
            dbgLog ("setStateInformation: loaded custom wavetable | paramWhenCustomLoaded=" + juce::String (paramWhenCustomLoaded));
        }
        else
        {
            // File gone — restore standard waveform
            customWavetablePath = {};
            oscillator.setWaveform (static_cast<WaveformType> (waveIdx + 1));
            paramWhenCustomLoaded = -1;
        }
    }
    else
    {
        oscillator.setWaveform (static_cast<WaveformType> (waveIdx + 1));
        paramWhenCustomLoaded = -1;
    }

    // OSC 2 custom wavetable
    customWavetable2Path = xml->getStringAttribute ("customWavetablePath2", {});
    if (customWavetable2Path.isNotEmpty())
    {
        juce::File f2 (customWavetable2Path);
        if (f2.existsAsFile())
        {
            if (f2.hasFileExtension (".wt")) osc2.loadWTFile (f2);
            else                             osc2.loadFromFile (f2);
            param2WhenCustomLoaded = (int) apvts.getRawParameterValue ("osc2Waveform")->load();
        }
        else
        {
            customWavetable2Path = {};
            const int w2 = (int) apvts.getRawParameterValue ("osc2Waveform")->load();
            osc2.setWaveform (static_cast<WaveformType> (w2 + 1));
            param2WhenCustomLoaded = -1;
        }
    }
    else
    {
        const int w2 = (int) apvts.getRawParameterValue ("osc2Waveform")->load();
        osc2.setWaveform (static_cast<WaveformType> (w2 + 1));
        param2WhenCustomLoaded = -1;
    }

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
