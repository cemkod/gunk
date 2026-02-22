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
    bool loadFromFile (const juce::File& file); // returns false on failure

    void  setFrequency (double freq, double sampleRate);
    void  setUnisonParams (int voices, float detuneCents, float blend);
    float getNextSample();
    float getNextSampleUnison();
    void  reset();

    WaveformType getCurrentWaveform() const { return currentWaveform; }

private:
    void buildSineTable();
    void buildTriangleTable();
    void buildSquareTable();
    void buildSawtoothTable();
    void recomputeVoiceIncrements();

    std::vector<float> wavetable; // kTableSize+1 entries (last == first for wrap)
    double phaseIndex    [kMaxVoices] = {};
    double phaseIncrement[kMaxVoices] = {};
    double currentBaseFreq   = 0.0;
    double currentSampleRate = 44100.0;
    int    unisonVoices      = 1;
    float  unisonDetuneCents = 0.0f;
    float  unisonBlend       = 0.5f;
    WaveformType currentWaveform = WaveformType::Sine;
    juce::SpinLock tableLock;    // guards wavetable during loadFromFile
};
