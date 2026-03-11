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
    std::vector<float> getFrameForDisplay (float morphPos) const;

    void  setFrequency (double freq, double sampleRate);
    void  setUnisonParams (int voices, float detuneCents, float blend);
    float getNextSample();
    // freqBuf: per-sample frequency in Hz (0 = silent/reset this sample)
    // out: filled with summed unison output, NOT scaled by level
    void  processBlock (const float* freqBuf, float* out, int n);
    void  reset();

    WaveformType getCurrentWaveform() const { return currentWaveform; }

private:
    void buildSineTable();
    void buildTriangleTable();
    void buildSquareTable();
    void buildSawtoothTable();
    void recomputeVoiceIncrements();

    // Multi-frame wavetable: frames[frameIdx][sample], each kTableSize+1 entries
    std::vector<std::vector<float>> frames;
    int   numFrames      = 1;
    float morphPosition  = 0.0f;  // 0..1

    double phaseIndex    [kMaxVoices] = {};
    double phaseIncrement[kMaxVoices] = {};
    double currentBaseFreq   = 0.0;
    double currentSampleRate = 44100.0;
    int    unisonVoices      = 1;
    float  unisonDetuneCents = 0.0f;
    float  unisonBlend       = 0.5f;
    WaveformType currentWaveform = WaveformType::Sine;
    juce::SpinLock tableLock;    // guards frames during load
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
