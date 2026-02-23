#pragma once

#include <JuceHeader.h>
#include "Oscillator.h"
#include "PitchDetector.h"
#include "FilterEngine.h"
#include "PresetManager.h"

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

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
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
    float getEnvelope() const          { return envelope; }
    float getCurrentCutoffHz() const   { return envelopeFilter.getCurrentCutoffHz(); }

    PresetManager& getPresetManager() { return presetManager; }
    void syncOscillatorAfterPresetLoad();

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // processBlock helpers
    void updateOscillatorParams();
    void updateGlideState (float detectedFreq, int glideSamples);

    PresetManager presetManager { apvts };

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
    static constexpr float kEnvAttack  = 0.010f;
    static constexpr float kEnvRelease = 0.100f;
    bool gateIsOpen = false;

    // Envelope filter (auto-wah)
    EnvelopeFilter envelopeFilter;

    double currentSampleRate = 48000;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JQGunkAudioProcessor)
};
