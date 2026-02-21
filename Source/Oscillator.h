#pragma once

//==============================================================================
// Simple phase-accumulator sawtooth oscillator.
class SawtoothOscillator
{
public:
    void setFrequency (double freq, double sampleRate);
    float getNextSample();
    void reset();

private:
    double phase = 0.0;
    double phaseIncrement = 0.0;
};
