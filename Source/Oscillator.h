#pragma once
#include <JuceHeader.h>
#include <vector>

enum class WaveformType { Sine = 0, Triangle = 1, Square = 2, Sawtooth = 3, Custom = 4 };

class WavetableOscillator
{
public:
    static constexpr int kTableSize  = 2048;
    static constexpr int kMaxVoices  = 8;

    WavetableOscillator();

    void setWaveform (WaveformType type);
    bool loadFromFile (const juce::File& file); // WAV, returns false on failure
    bool loadWTFile  (const juce::File& file);  // .wt binary format

    void  setMorph (float pos);                 // 0..1
    int   getNumFrames() const { return numFrames; }
    void getFrameForDisplay (float morphPos, std::vector<float>& out) const;

    void  setFrequency (double freq, double sampleRate);
    void  setUnisonParams (int voices, float detuneCents, float blend);
    float getNextSample();
    // freqBuf: per-sample frequency in Hz (0 = silent/reset this sample)
    // out: filled with summed unison output (the mid signal), NOT scaled by level
    // side: if non-null, filled with the side signal for the given stereo width
    //       (0..1), so that L = out + side and R = out - side. Voices are panned by
    //       their detune position with a linear law, so (L + R) / 2 == out at any
    //       width. Width ramps from the previous block's value across this block.
    void  processBlock (const float* freqBuf, float* out, int n,
                        float* side = nullptr, float width = 0.0f);
    void  reset();

    WaveformType getCurrentWaveform() const { return currentWaveform; }

private:
    // Multi-frame wavetable: frames[frameIdx][sample], each kTableSize+1 entries
    using Frames = std::vector<std::vector<float>>;

    // Built-in shapes are built once, shared by all instances and never freed, so
    // switching to one is a pointer swap that is safe on the audio thread.
    static const Frames& basicTable (WaveformType type);
    void recomputeVoiceIncrements();

    const Frames* frames = nullptr;  // points at a basic table or customFrames
    Frames customFrames;             // only replaced on the message thread, under tableLock
    int   numFrames      = 1;
    float morphPosition  = 0.0f;  // 0..1

    double phaseIndex    [kMaxVoices] = {};
    double phaseIncrement[kMaxVoices] = {};
    double currentBaseFreq   = 0.0;
    double currentSampleRate = 44100.0;
    int    unisonVoices      = 1;
    float  unisonDetuneCents = 0.0f;
    float  unisonBlend       = 0.5f;
    float  lastWidth         = 0.0f;
    WaveformType currentWaveform = WaveformType::Sine;
    juce::SpinLock tableLock;    // guards frames during load

    // frames may point into this object's own customFrames, so a copy would dangle.
    JUCE_DECLARE_NON_COPYABLE (WavetableOscillator)
};

class SineOscillator
{
public:
    void setFrequency (double freq, double sampleRate)
    {
        currentSampleRate = sampleRate;
        phaseIncrement = juce::MathConstants<double>::twoPi * freq / sampleRate;
    }
    float getNextSample()
    {
        float s = (float) std::sin (phase);
        phase  += phaseIncrement;
        if (phase >= juce::MathConstants<double>::twoPi)
            phase -= juce::MathConstants<double>::twoPi;
        return s;
    }
    void processBlock (const float* freqBuf, float* out, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            if (freqBuf[i] > 0.0f)
            {
                phaseIncrement = juce::MathConstants<double>::twoPi * freqBuf[i] / currentSampleRate;
                out[i] = (float) std::sin (phase);
                phase += phaseIncrement;
                if (phase >= juce::MathConstants<double>::twoPi)
                    phase -= juce::MathConstants<double>::twoPi;
            }
            else
            {
                phase  = 0.0;
                out[i] = 0.0f;
            }
        }
    }
    void reset() { phase = 0.0; }

private:
    double phase             = 0.0;
    double phaseIncrement    = 0.0;
    double currentSampleRate = 44100.0;
};
