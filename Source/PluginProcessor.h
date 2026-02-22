#pragma once

#include <JuceHeader.h>
#include "Oscillator.h"
#include "PitchDetector.h"
#include "Filter.h"

//==============================================================================
class JQGunkAudioProcessor : public juce::AudioProcessor
{
public:
    JQGunkAudioProcessor();
    ~JQGunkAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock; // expose the double-precision overload

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& dest) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    bool loadWavetableFromFile (const juce::File& file);

    bool isCustomWavetableLoaded() const { return customWavetablePath.isNotEmpty(); }
    bool isCustomWaveformActive() const;
    void reactivateCustomWavetable();

    float getDetectedFrequency() const { return detector.getFrequency(); }
    bool  isGateOpen() const           { return gateIsOpen; }

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    AutocorrelationPitchDetector detector;
    WavetableOscillator oscillator;
    WavetableOscillator subOscillator;
    juce::String customWavetablePath;
    int paramWhenCustomLoaded = -1; // waveform param index active when a WAV was loaded

    // Last successfully detected frequency — held until signal drops below gate
    float lastDetectedFreq = 0.0f;
    float glideFreq = 0.0f;
    float glideSourceFreq   = 0.0f;
    float glideTargetFreq   = 0.0f;
    int   glideSamplesElapsed = 0;
    int   glideSamplesTotal   = 0;
    int   glideSnapHops       = 0; // snap (no ramp) for first N detections after gate-open

    // Envelope follower for the noise gate
    float envelope = 0.0f;
    static constexpr float kEnvAttack  = 0.001f;
    static constexpr float kEnvRelease = 0.005f;
    bool gateIsOpen = false;

    // Envelope filter (auto-wah)
    ResonantLowpassFilter envFilter;
    float filterEnvelope = 0.0f;
    static constexpr float kFilterEnvAttack = 0.001f;

    double currentSampleRate = 48000;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JQGunkAudioProcessor)
};
