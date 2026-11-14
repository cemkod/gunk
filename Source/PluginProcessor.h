#pragma once

#include <JuceHeader.h>
#include "ParameterIDs.h"
#include "Oscillator.h"
#include "PitchDetectorThread.h"
#include "FilterEngine.h"
#include "GlideEngine.h"
#include "PresetManager.h"
#include "TransientPlayer.h"
#include "IRConvolutionEngine.h"
#include "DriveEngine.h"
#include "SideHighpass.h"
#include "ModMatrix.h"
#include "LFOEngine.h"
#include "AubioOnsetDetector.h"
#include "ParamCache.h"
#include "ADSREnvelope.h"
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
    double getTailLengthSeconds() const override;

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

    bool loadIRFromFile (const juce::File& file);
    juce::String getIRSamplePath() const;
    static juce::File getIRPresetsDir();
    static juce::File getTransientSamplesDir();

    int getOscNumFrames()  const { return oscillator.getNumFrames(); }
    int getOsc2NumFrames() const { return osc2.getNumFrames(); }

    void getOscFrameForDisplay (int oscIdx, std::vector<float>& out) const
    {
        float morph = (oscIdx == 0)
            ? lastModulatedMorph .load (std::memory_order_relaxed)
            : lastModulatedMorph2.load (std::memory_order_relaxed);
        (oscIdx == 0 ? oscillator : osc2).getFrameForDisplay (morph, out);
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
    float getModEnvelope()  const noexcept { return modEnvelope; }
    float getADSRVelocity() const noexcept { return adsrEnvelope.getVelocity(); }
    float getCurrentCutoffHz() const   { return envelopeFilter.getCurrentCutoffHz(); }
    bool  consumeTransient()           { return transientFlag.exchange (false, std::memory_order_relaxed); }
    float getLFOValue() const          { return lfo.getValue(); }

    PresetManager& getPresetManager() { return presetManager; }
    juce::StringArray syncOscillatorAfterPresetLoad();

    static juce::File getWavetablesDir();
    bool saveUserPreset (const juce::String& name);

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // All block-rate parameter snapshots and their derived values.
    struct BlockParams
    {
        bool  osc1Enabled;
        bool  osc2Enabled;
        float oscLevel;
        float dryLevel;
        float gateThresh;
        float openThresh;      // derived: gateThresh * 10^(gateHyst/20)
        bool  subEnabled;
        float subLevel;
        int   subOctaveIdx;
        float subOctaveMult;   // derived from subOctaveIdx
        bool  subBypassFilter;
        bool  filterEnabled;
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
        bool  transientEnabled;
        float transientLevel;
        float transientAttack;
        float transientHold;
        float transientDecay;
        float transientPitch;
        float transientDry;
        float modAttCoef;
        float modDecCoef;
        float modEnvSusLevel;
        float modEnvRelCoef;
        float transientThreshold;
    };

    BlockParams readBlockParams() const;

    // processBlock helpers
    struct OscUpdateConfig
    {
        WavetableOscillator&  osc;
        int&                  paramWhenCustomLoaded;
        std::atomic<float>*   waveformPtr;
        std::atomic<float>*   voicesPtr;
        std::atomic<float>*   detunePtr;
        std::atomic<float>*   blendPtr;
        std::atomic<float>*   morphPtr;
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
        juce::HeapBlock<float> irMixBuf;      // pre-IR full mix (dry+wet+transient), left/mono
        juce::HeapBlock<float> irMixBufR;     // pre-IR full mix, right
        juce::HeapBlock<float> osc1Side;      // osc1 side signal; reused as the summed side
        juce::HeapBlock<float> osc2Side;
        juce::HeapBlock<float> fxRight;       // right channel through drive and filter
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
            irMixBuf.realloc (maxBlockSize);    irMixBufR.realloc (maxBlockSize);
            osc1Side.realloc (maxBlockSize);    osc2Side.realloc (maxBlockSize);
            fxRight.realloc (maxBlockSize);
        }
    };

    void        updateTransientDetection (const BlockParams& p, float inputSample);
    void        restoreOscWavetable      (const juce::XmlElement* xml, int oscIdx,
                                           juce::StringArray& missingFiles);
    void        restoreSamplePaths       (const juce::XmlElement* xml,
                                           juce::StringArray* missingFiles);

    // Library-relative paths for presets: stored relative to libraryDir when the
    // file lives inside it, absolute otherwise. fromPresetPath accepts either.
    static juce::String toPresetPath   (const juce::String& absPath, const juce::File& libraryDir);
    static juce::String fromPresetPath (const juce::String& savedPath, const juce::File& libraryDir);

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

    DriveEngine         drive;
    IRConvolutionEngine irConvolution;
    juce::String        irSamplePath;

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

    // ADSR envelope state
    ADSREnvelope adsrEnvelope;
    bool      prevGateWasOpen    = false;
    bool      lastTransientNoteOn = false;

    // Transient detector
    AubioOnsetDetector onsetDetector;
    std::atomic<bool> transientFlag { false };

    // Envelope filter (auto-wah)
    EnvelopeFilter envelopeFilter;   // left / mono; drives the UI cutoff readout
    EnvelopeFilter envelopeFilterR;  // right, stereo output only
    SideHighpass   sideHighpass;

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
    int   perfBlockCount  = 0;
    float outMaxAbsEver   = 0.0f;  // peak |output| per PERF interval
    int   outNaNCount     = 0;     // non-finite output samples per interval
    static constexpr int kPerfPrintInterval = 500;
#endif

    ParamCache params;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JQGunkAudioProcessor)
};
