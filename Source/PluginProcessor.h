#pragma once

#include <JuceHeader.h>
#include "ParameterIDs.h"
#include "Oscillator.h"
#include "PitchDetectorThread.h"
#include "FilterEngine.h"
#include "GlideEngine.h"
#include "PresetManager.h"
#include "TransientPlayer.h"
#include "ModMatrix.h"
#include "LFOEngine.h"
#include "AubioOnsetDetector.h"
#include <q/fx/envelope.hpp>
#include <q/fx/noise_gate.hpp>
#include <q/support/literals.hpp>

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

    float getModulatedDetune (int oscIdx) const {
        const auto id = oscIdx == 0 ? ParamIDs::unisonDetune : ParamIDs::osc2UnisonDetune;
        const float base   = apvts.getRawParameterValue (id)->load();
        const float offset = (oscIdx == 0 ? lastModDetuneOffset : lastModDetune2Offset)
                                 .load (std::memory_order_relaxed);
        return juce::jlimit (0.0f, 100.0f, base + offset);
    }
    float getModulatedBlend (int oscIdx) const {
        const auto id = oscIdx == 0 ? ParamIDs::unisonBlend : ParamIDs::osc2UnisonBlend;
        const float base   = apvts.getRawParameterValue (id)->load();
        const float offset = (oscIdx == 0 ? lastModBlendOffset : lastModBlend2Offset)
                                 .load (std::memory_order_relaxed);
        return juce::jlimit (0.0f, 1.0f, base + offset);
    }

    float getDetectedFrequency() const { return pitchThread.getDetectedFrequency(); }
    bool  isGateOpen() const           { return gateIsOpen; }
    float getEnvelope() const          { return envelope; }
    float getModEnvelope() const noexcept { return modEnvelope; }
    float getCurrentCutoffHz() const   { return envelopeFilter.getCurrentCutoffHz(); }
    bool  consumeTransient()           { return transientFlag.exchange (false, std::memory_order_relaxed); }
    float getLFOValue() const          { return lfo.getValue(); }

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
        float transientHold;
        float transientDecay;
        float transientPitch;
        float transientDry;
        float modAttCoef;
        float modDecCoef;
        float transientThreshold;
    };

    BlockParams readBlockParams() const;

    // processBlock helpers
    struct OscUpdateConfig
    {
        WavetableOscillator&  osc;
        int&                  paramWhenCustomLoaded;
        const char*           waveformId;
        const char*           voicesId;
        const char*           detuneId;
        const char*           blendId;
        const char*           morphId;
        ModTarget             detuneTarget;
        ModTarget             blendTarget;
        ModTarget             morphTarget;
        std::atomic<float>&   lastDetuneOffset;
        std::atomic<float>&   lastBlendOffset;
        std::atomic<float>&   lastMorphModulated;
    };

    void updateOscParams (const OscUpdateConfig& cfg);

    // Per-block scratch buffers for block-based audio processing.
    struct AudioScratch
    {
        juce::HeapBlock<float> glidedFreq;    // per-sample glided osc1 freq (0 = gate closed)
        juce::HeapBlock<float> osc2Freq;      // per-sample osc2 freq
        juce::HeapBlock<float> subFreq;       // per-sample sub freq
        juce::HeapBlock<float> envelopeBuf;   // per-sample cycfi Q envelope output
        juce::HeapBlock<float> modEnvBuf;     // per-sample slewed mod envelope
        juce::HeapBlock<float> lfoBuf;        // per-sample LFO output
        juce::HeapBlock<float> filterCutoff;  // per-sample computed cutoff Hz
        juce::HeapBlock<float> osc1Out;
        juce::HeapBlock<float> osc2Out;
        juce::HeapBlock<float> subOut;
        juce::HeapBlock<float> transientOut;
        juce::HeapBlock<float> transientEnv;  // transient player envelope (for dry gating)
        juce::HeapBlock<float> gatedInput;    // gated copy of input for pitch thread
        juce::HeapBlock<float> pitchNorm;     // per-sample normalized pitch for mod matrix
        int capacity = 0;

        void prepare (int maxBlockSize)
        {
            if (maxBlockSize <= capacity) return;
            capacity = maxBlockSize;
            glidedFreq.realloc (maxBlockSize);   osc2Freq.realloc (maxBlockSize);
            subFreq.realloc (maxBlockSize);      envelopeBuf.realloc (maxBlockSize);
            modEnvBuf.realloc (maxBlockSize);    lfoBuf.realloc (maxBlockSize);
            filterCutoff.realloc (maxBlockSize); osc1Out.realloc (maxBlockSize);
            osc2Out.realloc (maxBlockSize);      subOut.realloc (maxBlockSize);
            transientOut.realloc (maxBlockSize); transientEnv.realloc (maxBlockSize);
            gatedInput.realloc (maxBlockSize);   pitchNorm.realloc (maxBlockSize);
        }
    };

    float       updateModEnvelopeSample  (float envSample, const BlockParams& p);
    void        updateTransientDetection (const BlockParams& p, float inputSample);
    void        restoreOscWavetable      (const juce::XmlElement* xml, int oscIdx);

    PresetManager presetManager { apvts };

    PitchDetectorThread pitchThread;
    WavetableOscillator oscillator;
    SineOscillator subOscillator;
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
    std::atomic<float> lastModDetuneOffset  { 0.0f };
    std::atomic<float> lastModDetune2Offset { 0.0f };
    std::atomic<float> lastModBlendOffset   { 0.0f };
    std::atomic<float> lastModBlend2Offset  { 0.0f };

    // Envelope follower and noise gate (Cycfi Q)
    std::unique_ptr<cycfi::q::ar_envelope_follower> qEnvFollower;
    std::unique_ptr<cycfi::q::noise_gate>           qGate;
    float envelope  = 0.0f;
    bool  gateIsOpen = false;

    // Slew-limited modulation envelope (driven by modEnvAttack/modEnvDecay params)
    float modEnvelope = 0.0f;

    // Transient detector
    AubioOnsetDetector onsetDetector;
    std::atomic<bool> transientFlag { false };

    // Envelope filter (auto-wah)
    EnvelopeFilter envelopeFilter;

    double currentSampleRate = 48000;

    AudioScratch scratch;

    LFOEngine lfo;


#ifdef ENABLE_DEBUG_LOG
    struct PerfStats
    {
        juce::int64 maxTicks   = 0;
        juce::int64 totalTicks = 0;
        int         count      = 0;
        juce::int64 startTick  = 0;

        void start() noexcept { startTick = juce::Time::getHighResolutionTicks(); }
        void stop()  noexcept
        {
            const juce::int64 e = juce::Time::getHighResolutionTicks() - startTick;
            if (e > maxTicks) maxTicks = e;
            totalTicks += e;
            ++count;
        }
        double maxMs() const noexcept { return juce::Time::highResolutionTicksToSeconds (maxTicks) * 1000.0; }
        double avgMs() const noexcept
        {
            return count > 0 ? (juce::Time::highResolutionTicksToSeconds (totalTicks) * 1000.0 / count) : 0.0;
        }
        void reset() noexcept { maxTicks = 0; totalTicks = 0; count = 0; }
    };
    PerfStats perfSetup, perfLoop, perfOnset;
    PerfStats perfOsc1, perfOsc2, perfSub, perfFilter;
    PerfStats perfTransient, perfModMatrix, perfControl, perfMix;
    int perfBlockCount = 0;
    static constexpr int kPerfPrintInterval = 500;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JQGunkAudioProcessor)
};
