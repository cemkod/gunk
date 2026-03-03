#pragma once

#include <JuceHeader.h>
#include "Oscillator.h"
#include "PitchDetector.h"
#include "FilterEngine.h"
#include "GlideEngine.h"
#include "PresetManager.h"
#include "TransientPlayer.h"
#include "ModMatrix.h"

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

    bool loadWavetable2FromFile (const juce::File& file);
    bool isCustomWavetable2Loaded() const { return customWavetable2Path.isNotEmpty(); }
    bool isCustomWaveform2Active() const;
    void reactivateCustomWavetable2();

    bool loadTransientSampleFromFile (const juce::File& file);
    bool isTransientSampleLoaded() const;
    juce::String getTransientSamplePath() const;

    int getOscNumFrames()  const { return oscillator.getNumFrames(); }
    int getOsc2NumFrames() const { return osc2.getNumFrames(); }

    std::vector<float> getOscFrameForDisplay (int oscIdx) const
    {
        float morph = (oscIdx == 0)
            ? lastModulatedMorph .load (std::memory_order_relaxed)
            : lastModulatedMorph2.load (std::memory_order_relaxed);
        return (oscIdx == 0 ? oscillator : osc2).getFrameForDisplay (morph);
    }

    juce::String getOscWavetableName (int oscIdx) const
    {
        const auto& p = (oscIdx == 0 ? customWavetablePath : customWavetable2Path);
        return p.isEmpty() ? juce::String{} : juce::File (p).getFileNameWithoutExtension();
    }

    float getDetectedFrequency() const { return detector.getFrequency(); }
    bool  isGateOpen() const           { return gateIsOpen; }
    float getEnvelope() const          { return envelope; }
    float getModEnvelope() const noexcept { return modEnvelope; }
    float getCurrentCutoffHz() const   { return envelopeFilter.getCurrentCutoffHz(); }
    bool  consumeTransient()           { return transientFlag.exchange (false, std::memory_order_relaxed); }
    float getLFOValue() const          { return lfoValueAtomic.load (std::memory_order_relaxed); }

    PresetManager& getPresetManager() { return presetManager; }
    void syncOscillatorAfterPresetLoad();

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // All block-rate parameter snapshots and their derived values.
    struct BlockParams
    {
        float oscLevel;
        float dryLevel;
        float gateThresh;
        float openThresh;      // derived: gateThresh * 10^(gateHyst/20)
        float attackCoeff;     // derived from kEnvAttack  + currentSampleRate
        float releaseCoeff;    // derived from kEnvRelease + currentSampleRate
        float subLevel;
        int   subOctaveIdx;
        float subOctaveMult;   // derived from subOctaveIdx
        bool  subBypassFilter;
        float resonance;
        int   filterType;
        float freqTracking;
        float filterFreq;
        float glideTime;
        int   glideSamples;    // derived from glideTime + currentSampleRate
        int   octaveShift;
        float osc2Level;
        int   osc2OctaveShift;
        float osc1PitchMult;   // derived: pow(2, (coarseTune + fineTune/100) / 12)
        float osc2PitchMult;   // derived: pow(2, (osc2CoarseTune + osc2FineTune/100) / 12)
        float transientLevel;
        float transientAttack;
        float transientDecay;
        float transientPitch;
    };

    BlockParams readBlockParams() const;

    // processBlock helpers
    void updateOscillatorParams();
    void updateOsc2Params();

    PresetManager presetManager { apvts };

    AutocorrelationPitchDetector detector;
    WavetableOscillator oscillator;
    WavetableOscillator subOscillator;
    juce::String customWavetablePath;
    int paramWhenCustomLoaded = -1; // waveform param index active when a WAV was loaded

    WavetableOscillator osc2;
    juce::String        customWavetable2Path;
    int                 param2WhenCustomLoaded = -1;

    TransientPlayer transientPlayer;
    juce::String    transientSamplePath;

    GlideEngine glide;
    ModMatrix   modMatrix;

    std::atomic<float> lastModulatedMorph  { 0.0f };
    std::atomic<float> lastModulatedMorph2 { 0.0f };

    // Envelope follower for the noise gate
    float envelope = 0.0f;
    static constexpr float kEnvAttack  = 0.010f;
    static constexpr float kEnvRelease = 0.100f;
    bool gateIsOpen = false;

    // Slew-limited modulation envelope (driven by modEnvAttack/modEnvDecay params)
    float modEnvelope = 0.0f;

    // Transient detector
    static constexpr int kSlopeLookback = 3; 
    std::array<float, kSlopeLookback> envHistory {};
    int   envHistoryPos = 0;
    int   transientCooldown = 0;
    std::atomic<bool> transientFlag { false };

    // Envelope filter (auto-wah)
    EnvelopeFilter envelopeFilter;

    double currentSampleRate = 48000;

    // LFO state
    float lfoPhase            = 0.0f;
    float lfoModulatedRate    = 1.0f;  // persists lfoFreq after LfoRate mod target
    float lfoModulatedAmount  = 1.0f;  // persists lfoAmount after LfoAmount mod target
    std::atomic<float> lfoValueAtomic { 0.0f };

    // Low-pass filter applied to input before pitch detection (~500 Hz cutoff)
    juce::dsp::IIR::Filter<float> pitchDetectorLPF;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JQGunkAudioProcessor)
};
