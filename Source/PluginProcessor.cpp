#include "PluginProcessor.h"
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
        0.01f,
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
        "glide", "Glide",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        timeFmt(), timeParse (0.0f, 1.0f)));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "oscLevel", "OSC Level",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 1.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        pctFmt(), pctParse (0.0f, 1.0f)));

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
        "envSensitivity", "Env Sensitivity",
        juce::NormalisableRange<float> (0.0f, 7.0f, 0.01f), 3.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        scaledPctFmt (7.0f), scaledPctParse (0.0f, 7.0f)));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "envResonance", "Env Resonance",
        juce::NormalisableRange<float> (0.0f, 8.0f, 0.01f), 2.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        scaledPctFmt (8.0f), scaledPctParse (0.0f, 8.0f)));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "envDecay", "Env Decay",
        juce::NormalisableRange<float> (0.01f, 2.0f, 0.001f), 0.3f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        timeFmt(), timeParse (0.01f, 2.0f)));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        "sweepMode", "Sweep",
        juce::StringArray { "Off", "Up", "Down" }, 1)); // default: Up

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "freqTracking", "Freq Tracking",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        pctFmt(), pctParse (0.0f, 1.0f)));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "filterFreq", "Filter Freq",
        juce::NormalisableRange<float> (-2000.0f, 4000.0f), 0.0f,
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
        juce::StringArray { "0", "+1", "+2" }, 0));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        "subOctave", "Sub Octave",
        juce::StringArray { "-2", "-1", "0", "+1" }, 1)); // default: -1

    layout.add (std::make_unique<juce::AudioParameterBool> (
        "subBypassFilter", "Sub Bypass Filter", true));

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
    envelope = 0.0f;
    glide.reset();
    gateIsOpen = false;
    envelopeFilter.reset();
    envelopeFilter.prepare (sampleRate, 0.3f); // default decay; updated per-block

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
void JQGunkAudioProcessor::updateOscillatorParams()
{
    const int waveIdx = (int) apvts.getRawParameterValue ("waveform")->load();
    // Param choices are Triangle/Square/Sawtooth (0/1/2); add 1 to map to WaveformType enum
    const auto requested = static_cast<WaveformType> (waveIdx + 1);
    // If a custom WAV is active, only switch away when the user picks a different
    // waveform (i.e. the parameter has changed since the WAV was loaded).
    const bool customActive = (oscillator.getCurrentWaveform() == WaveformType::Custom);
    if (!customActive || waveIdx != paramWhenCustomLoaded)
    {
        if (oscillator.getCurrentWaveform() != requested)
            oscillator.setWaveform (requested);
    }

    const int   numVoices   = juce::roundToInt (apvts.getRawParameterValue ("unisonVoices")->load());
    const float detuneCents = apvts.getRawParameterValue ("unisonDetune")->load();
    const float uniBlend    = apvts.getRawParameterValue ("unisonBlend")->load();
    oscillator.setUnisonParams (numVoices, detuneCents, uniBlend);
}

//==============================================================================
void JQGunkAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    const float oscLevel = apvts.getRawParameterValue ("oscLevel")->load();
    const float dryLevel = apvts.getRawParameterValue ("dryLevel")->load();

    updateOscillatorParams();

    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    // Use channel 0 as the mono analysis source
    const float* inputData = buffer.getReadPointer (0);

    const float gateThresh = apvts.getRawParameterValue ("gateThreshold")->load();
    const float gateHyst   = apvts.getRawParameterValue ("gateHysteresis")->load();

    const float subLevel         = apvts.getRawParameterValue ("subLevel")->load();
    const int   subOctaveIdx     = (int) apvts.getRawParameterValue ("subOctave")->load();
    const bool  subBypassFilter  = apvts.getRawParameterValue ("subBypassFilter")->load() > 0.5f;
    const float subOctaveMult    = (subOctaveIdx == 0) ? 0.25f
                                 : (subOctaveIdx == 1) ? 0.5f
                                 : (subOctaveIdx == 2) ? 1.0f
                                 :                       2.0f;
    const float sensitivity   = apvts.getRawParameterValue ("envSensitivity")->load();
    const float resonance     = apvts.getRawParameterValue ("envResonance")->load();
    const float decay         = apvts.getRawParameterValue ("envDecay")->load();
    const float freqTracking  = apvts.getRawParameterValue ("freqTracking")->load();
    const float filterFreq    = apvts.getRawParameterValue ("filterFreq")->load();

    // Envelope follower attack/release coefficients
    const float attackCoeff  = 1.0f - std::exp (-1.0f / (float) (currentSampleRate * kEnvAttack));
    const float releaseCoeff = 1.0f - std::exp (-1.0f / (float) (currentSampleRate * kEnvRelease));

    envelopeFilter.prepare (currentSampleRate, decay); // updates decay coeff each block

    const float glideTime  = apvts.getRawParameterValue ("glide")->load();
    const int   sweepMode  = (int) apvts.getRawParameterValue ("sweepMode")->load();

    // Schmitt trigger: open threshold is close threshold raised by hysteresis dB
    const float openThresh = gateThresh * std::pow (10.0f, gateHyst / 20.0f);

    for (int i = 0; i < numSamples; ++i)
    {
        const float inputSample = inputData[i];
        const float absSample   = std::abs (inputSample);

        // Envelope follower
        if (absSample > envelope)
            envelope += attackCoeff  * (absSample - envelope);
        else
            envelope += releaseCoeff * (absSample - envelope);

        // Schmitt trigger gate
        if (!gateIsOpen && envelope >= openThresh)
            gateIsOpen = true;
        else if (gateIsOpen && envelope < gateThresh)
            gateIsOpen = false;

        // Gate: clear stale pitch when gate is closed
        if (!gateIsOpen)
            detector.clearHistory();

        float filteredForPitch = pitchDetectorLPF.processSample (inputSample);
        float detectedFreq = detector.processSample (filteredForPitch, currentSampleRate);

        const int octaveIdx = (int) apvts.getRawParameterValue ("octaveShift")->load();
        if (octaveIdx > 0)
            detectedFreq *= (octaveIdx == 1 ? 2.0f : 4.0f);

        const int glideSamples = (glideTime > 0.0f)
            ? (int) (currentSampleRate * glideTime)
            : 0;

        glide.update (detectedFreq, glideSamples, gateIsOpen,
                      oscillator, subOscillator, currentSampleRate, subOctaveMult);

        // Generate oscillator sample; optionally apply envelope filter
        const float sawSample = oscillator.getNextSampleUnison();
        const float subSample = subOscillator.getNextSample();

        // subBypassFilter=true: sub added after filter (default, bypasses auto-wah)
        // subBypassFilter=false: sub mixed into filter input
        const float filterInput = subBypassFilter ? sawSample : (sawSample + subSample * subLevel);
        float filteredSample = envelopeFilter.processSample (
            inputSample, filterInput, glide.lastDetectedFreq,
            filterFreq, freqTracking, sensitivity, resonance, sweepMode);

        const float oscWet = filteredSample * envelope * oscLevel;
        const float subWet = subBypassFilter ? subSample * subLevel * envelope : 0.0f;
        const float wet = oscWet + subWet;
        const float dry = inputSample * dryLevel;
        const float out = dry + wet;

        for (int ch = 0; ch < numChannels; ++ch)
            buffer.getWritePointer (ch)[i] = out;
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
bool JQGunkAudioProcessor::loadWavetableFromFile (const juce::File& file)
{
    if (! oscillator.loadFromFile (file)) return false;
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
            oscillator.loadFromFile (f);
            paramWhenCustomLoaded = waveIdx;
            dbgLog ("setStateInformation: loaded custom wavetable | paramWhenCustomLoaded=" + juce::String (paramWhenCustomLoaded));
            return;
        }
        // File gone — fall through and restore standard waveform
        customWavetablePath = {};
    }

    // No custom wavetable: sync oscillator immediately so state is consistent
    oscillator.setWaveform (static_cast<WaveformType> (waveIdx + 1));
    paramWhenCustomLoaded = -1;
    dbgLog ("setStateInformation: set standard waveform | waveIdx=" + juce::String (waveIdx)
            + " | osc=" + juce::String ((int) oscillator.getCurrentWaveform()));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new JQGunkAudioProcessor();
}
