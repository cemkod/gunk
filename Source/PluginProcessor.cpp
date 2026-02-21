#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
BassSynthAudioProcessor::BassSynthAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

BassSynthAudioProcessor::~BassSynthAudioProcessor() {}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
BassSynthAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "level", "Output Level",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "mix", "Dry/Wet Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 1.0f));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        "waveform", "Waveform",
        juce::StringArray { "Sine", "Triangle", "Square", "Sawtooth" },
        0)); // default: Sine

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "envSensitivity", "Env Sensitivity",
        juce::NormalisableRange<float> (1.0f, 10.0f, 0.01f), 3.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "envResonance", "Env Resonance",
        juce::NormalisableRange<float> (0.5f, 8.0f, 0.01f), 2.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "envDecay", "Env Decay",
        juce::NormalisableRange<float> (0.01f, 2.0f, 0.001f), 0.3f));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        "sweepMode", "Sweep",
        juce::StringArray { "Off", "Up", "Down" }, 1)); // default: Up

    return layout;
}

//==============================================================================
void BassSynthAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;
    detector.reset();
    detector.setSampleRate (sampleRate);
    oscillator.reset();
    envelope = 0.0f;
    filterEnvelope = 0.0f;
    envFilter.reset();
}

void BassSynthAudioProcessor::releaseResources() {}

bool BassSynthAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

//==============================================================================
void BassSynthAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    const float level     = apvts.getRawParameterValue ("level")->load();
    const float mix       = apvts.getRawParameterValue ("mix")->load();

    const int waveIdx = (int) apvts.getRawParameterValue ("waveform")->load();
    const auto requested = static_cast<WaveformType> (waveIdx);
    // If a custom WAV is active, only switch away when the user picks a different
    // waveform (i.e. the parameter has changed since the WAV was loaded).
    const bool customActive = (oscillator.getCurrentWaveform() == WaveformType::Custom);
    if (!customActive || waveIdx != paramWhenCustomLoaded)
    {
        if (oscillator.getCurrentWaveform() != requested)
            oscillator.setWaveform (requested);
    }

    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    // Use channel 0 as the mono analysis source
    const float* inputData = buffer.getReadPointer (0);

    const float sensitivity = apvts.getRawParameterValue ("envSensitivity")->load();
    const float resonance   = apvts.getRawParameterValue ("envResonance")->load();
    const float decay       = apvts.getRawParameterValue ("envDecay")->load();

    // Envelope follower attack/release coefficients
    const float attackCoeff  = 1.0f - std::exp (-1.0f / (float) (currentSampleRate * kEnvAttack));
    const float releaseCoeff = 1.0f - std::exp (-1.0f / (float) (currentSampleRate * kEnvRelease));

    const float filterAttackCoeff = 1.0f - std::exp (-1.0f / (float) (currentSampleRate * kFilterEnvAttack));
    const float filterDecayCoeff  = 1.0f - std::exp (-1.0f / (float) (currentSampleRate * decay));



    for (int i = 0; i < numSamples; ++i)
    {
        const float inputSample = inputData[i];
        const float absSample   = std::abs (inputSample);

        // Envelope follower
        if (absSample > envelope)
            envelope += attackCoeff  * (absSample - envelope);
        else
            envelope += releaseCoeff * (absSample - envelope);


        // Gate: clear stale pitch when signal is below noise floor
        if (envelope < kGateThreshold)
            detector.clearHistory();

        // Filter envelope follower (fast fixed attack, user-controlled decay)
        if (absSample > filterEnvelope)
            filterEnvelope += filterAttackCoeff * (absSample - filterEnvelope);
        else
            filterEnvelope += filterDecayCoeff  * (absSample - filterEnvelope);

        float detectedFreq = detector.processSample (inputSample, currentSampleRate);

        if (detectedFreq > 0.0f)
            oscillator.setFrequency (detectedFreq, currentSampleRate);
        else
            oscillator.reset();

        // Generate oscillator sample; optionally apply envelope filter
        const float sawSample = oscillator.getNextSample();
        const int sweepMode = (int) apvts.getRawParameterValue ("sweepMode")->load();
        float filteredSample = sawSample;
        if (sweepMode != 0)
        {
            float envControlled = juce::jlimit (0.0f, 1.0f, filterEnvelope * sensitivity);
            float sweepParam = (sweepMode == 2) ? (1.0f - envControlled) : envControlled;
            float cutoff = 200.0f * std::pow (20.0f, sweepParam); // 200–4000 Hz
            filteredSample = envFilter.process (sawSample, cutoff, resonance, (float) currentSampleRate);
        }

        // Blend dry/wet and apply output level
        const float wet = filteredSample * envelope * mix;
        const float dry = inputSample * (1.0f - mix);
        const float out = (dry + wet) * level;

        for (int ch = 0; ch < numChannels; ++ch)
            buffer.getWritePointer (ch)[i] = out;
    }
}

//==============================================================================
bool BassSynthAudioProcessor::loadWavetableFromFile (const juce::File& file)
{
    if (! oscillator.loadFromFile (file)) return false;
    customWavetablePath = file.getFullPathName();
    paramWhenCustomLoaded = (int) apvts.getRawParameterValue ("waveform")->load();
    return true;
}

//==============================================================================
juce::AudioProcessorEditor* BassSynthAudioProcessor::createEditor()
{
    return new BassSynthAudioProcessorEditor (*this);
}

void BassSynthAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    xml->setAttribute ("customWavetablePath", customWavetablePath);
    copyXmlToBinary (*xml, dest);
}

void BassSynthAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));

    customWavetablePath = xml->getStringAttribute ("customWavetablePath", {});
    if (customWavetablePath.isNotEmpty())
    {
        juce::File f (customWavetablePath);
        if (f.existsAsFile())
        {
            oscillator.loadFromFile (f);
            paramWhenCustomLoaded = (int) apvts.getRawParameterValue ("waveform")->load();
        }
    }
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BassSynthAudioProcessor();
}
