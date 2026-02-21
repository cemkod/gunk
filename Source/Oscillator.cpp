#include "Oscillator.h"

WavetableOscillator::WavetableOscillator()
{
    wavetable.resize (kTableSize + 1, 0.0f);
    buildSineTable();
}

//==============================================================================
void WavetableOscillator::buildSineTable()
{
    for (int i = 0; i < kTableSize; ++i)
        wavetable[(size_t) i] = std::sin (2.0f * juce::MathConstants<float>::pi * (float) i / (float) kTableSize);
    wavetable[(size_t) kTableSize] = wavetable[0];
    currentWaveform = WaveformType::Sine;
}

void WavetableOscillator::buildTriangleTable()
{
    for (int i = 0; i < kTableSize; ++i)
    {
        float t = (float) i / (float) kTableSize;
        wavetable[(size_t) i] = (t < 0.5f) ? (4.0f * t - 1.0f) : (3.0f - 4.0f * t);
    }
    wavetable[(size_t) kTableSize] = wavetable[0];
    currentWaveform = WaveformType::Triangle;
}

void WavetableOscillator::buildSquareTable()
{
    for (int i = 0; i < kTableSize; ++i)
        wavetable[(size_t) i] = (i < kTableSize / 2) ? 1.0f : -1.0f;
    wavetable[(size_t) kTableSize] = wavetable[0];
    currentWaveform = WaveformType::Square;
}

void WavetableOscillator::buildSawtoothTable()
{
    for (int i = 0; i < kTableSize; ++i)
        wavetable[(size_t) i] = (float) (2.0 * i / kTableSize - 1.0);
    wavetable[(size_t) kTableSize] = wavetable[0];
    currentWaveform = WaveformType::Sawtooth;
}

//==============================================================================
void WavetableOscillator::setWaveform (WaveformType type)
{
    juce::SpinLock::ScopedLockType sl (tableLock);
    switch (type)
    {
        case WaveformType::Sine:     buildSineTable();     break;
        case WaveformType::Triangle: buildTriangleTable(); break;
        case WaveformType::Square:   buildSquareTable();   break;
        case WaveformType::Sawtooth: buildSawtoothTable(); break;
        case WaveformType::Custom:   break; // only settable via loadFromFile
    }
    // phaseIndex intentionally not reset to avoid click mid-playback
}

bool WavetableOscillator::loadFromFile (const juce::File& file)
{
    juce::AudioFormatManager mgr;
    mgr.registerFormat (new juce::WavAudioFormat(), true);

    std::unique_ptr<juce::AudioFormatReader> reader (mgr.createReaderFor (file));
    if (reader == nullptr || reader->lengthInSamples < 2)
        return false;

    const auto numSamplesToRead = juce::jmin (reader->lengthInSamples, (juce::int64) 48000 * 600);

    juce::AudioBuffer<float> tempBuffer (1, (int) numSamplesToRead);
    reader->read (&tempBuffer, 0, (int) numSamplesToRead, 0, true, false);

    const float* src = tempBuffer.getReadPointer (0);
    const int srcLen = (int) numSamplesToRead;

    std::vector<float> newTable ((size_t) kTableSize + 1);
    for (int i = 0; i < kTableSize; ++i)
    {
        double pos  = (double) i * (srcLen - 1) / kTableSize;
        int    idx  = (int) pos;
        double frac = pos - idx;
        newTable[(size_t) i] = (float) ((1.0 - frac) * src[idx] + frac * src[idx + 1]);
    }
    newTable[(size_t) kTableSize] = newTable[0];

    juce::SpinLock::ScopedLockType sl (tableLock);
    std::swap (wavetable, newTable);
    currentWaveform = WaveformType::Custom;
    return true;
}

//==============================================================================
void WavetableOscillator::setFrequency (double freq, double sampleRate)
{
    phaseIncrement = kTableSize * freq / sampleRate;
}

float WavetableOscillator::getNextSample()
{
    juce::SpinLock::ScopedTryLockType sl (tableLock);
    if (! sl.isLocked())
        return 0.0f;

    auto   i    = (size_t) phaseIndex;
    double frac = phaseIndex - (double) i;
    float  s    = (float) ((1.0 - frac) * wavetable[i] + frac * wavetable[i + 1]);

    phaseIndex += phaseIncrement;
    if (phaseIndex >= kTableSize)
        phaseIndex -= kTableSize;

    return s;
}

void WavetableOscillator::reset()
{
    phaseIndex     = 0.0;
    phaseIncrement = 0.0;
}
