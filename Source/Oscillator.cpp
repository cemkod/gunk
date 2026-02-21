#include "Oscillator.h"

void SawtoothOscillator::setFrequency (double freq, double sampleRate)
{
    if (phaseIncrement <= 0.0)
        phase = 0.5; // zero crossing — avoids click on first sample
    phaseIncrement = freq / sampleRate;
}

float SawtoothOscillator::getNextSample()
{
    if (phaseIncrement <= 0.0)
        return 0.0f;
    float output = (float) (2.0 * phase - 1.0);
    phase += phaseIncrement;
    if (phase >= 1.0)
        phase -= 1.0;
    return output;
}

void SawtoothOscillator::reset()
{
    phase = 0.0;
    phaseIncrement = 0.0;
}
