#pragma once
#include <JuceHeader.h>
#include <vector>

enum class WaveformType { Sine = 0, Triangle = 1, Square = 2, Sawtooth = 3, Custom = 4 };

class WavetableOscillator
{
public:
    static constexpr int kTableSize = 2048;

    WavetableOscillator();

    void setWaveform (WaveformType type);
    bool loadFromFile (const juce::File& file); // returns false on failure

    void  setFrequency (double freq, double sampleRate);
    float getNextSample();
    void  reset();

    WaveformType getCurrentWaveform() const { return currentWaveform; }

private:
    void buildSineTable();
    void buildTriangleTable();
    void buildSquareTable();
    void buildSawtoothTable();

    std::vector<float> wavetable; // kTableSize+1 entries (last == first for wrap)
    double phaseIndex     = 0.0;
    double phaseIncrement = 0.0;
    WaveformType currentWaveform = WaveformType::Sine;
    juce::SpinLock tableLock;    // guards wavetable during loadFromFile
};
