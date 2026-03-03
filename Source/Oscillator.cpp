#include "Oscillator.h"

static constexpr double    kCentsPerOctave    = 1200.0;
static constexpr juce::int64 kMaxWavReadSamples = (juce::int64) 384000 * 600;

WavetableOscillator::WavetableOscillator()
{
    frames.resize (1, std::vector<float> ((size_t) kTableSize + 1, 0.0f));
    buildSineTable();
}

//==============================================================================
void WavetableOscillator::buildSineTable()
{
    frames.resize (1);
    frames[0].resize ((size_t) kTableSize + 1);
    for (int i = 0; i < kTableSize; ++i)
        frames[0][(size_t) i] = std::sin (2.0f * juce::MathConstants<float>::pi * (float) i / (float) kTableSize);
    frames[0][(size_t) kTableSize] = frames[0][0];
    numFrames = 1;
    currentWaveform = WaveformType::Sine;
}

void WavetableOscillator::buildTriangleTable()
{
    frames.resize (1);
    frames[0].resize ((size_t) kTableSize + 1);
    for (int i = 0; i < kTableSize; ++i)
    {
        float t = (float) i / (float) kTableSize;
        frames[0][(size_t) i] = (t < 0.5f) ? (4.0f * t - 1.0f) : (3.0f - 4.0f * t);
    }
    frames[0][(size_t) kTableSize] = frames[0][0];
    numFrames = 1;
    currentWaveform = WaveformType::Triangle;
}

void WavetableOscillator::buildSquareTable()
{
    frames.resize (1);
    frames[0].resize ((size_t) kTableSize + 1);
    for (int i = 0; i < kTableSize; ++i)
        frames[0][(size_t) i] = (i < kTableSize / 2) ? 1.0f : -1.0f;
    frames[0][(size_t) kTableSize] = frames[0][0];
    numFrames = 1;
    currentWaveform = WaveformType::Square;
}

void WavetableOscillator::buildSawtoothTable()
{
    frames.resize (1);
    frames[0].resize ((size_t) kTableSize + 1);
    for (int i = 0; i < kTableSize; ++i)
        frames[0][(size_t) i] = (float) (2.0 * i / kTableSize - 1.0);
    frames[0][(size_t) kTableSize] = frames[0][0];
    numFrames = 1;
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
        case WaveformType::Custom:   break; // only settable via loadFromFile / loadWTFile
    }
    // phaseIndex intentionally not reset to avoid click mid-playback
}

static int detectWAVFrameSize (const juce::File& file)
{
    juce::FileInputStream stream (file);
    if (! stream.openedOk()) return 0;

    // Validate RIFF/WAVE header (12 bytes)
    char riff[4], wave[4];
    stream.read (riff, 4);
    stream.skipNextBytes (4); // file size field
    stream.read (wave, 4);
    if (memcmp (riff, "RIFF", 4) != 0 || memcmp (wave, "WAVE", 4) != 0)
        return 0;

    // Iterate RIFF chunks
    while (! stream.isExhausted())
    {
        char id[4] = {};
        int32_t chunkSize = 0;
        if (stream.read (id, 4) != 4) break;
        stream.read (&chunkSize, 4);
        if (chunkSize < 0) break;
        // RIFF chunks are padded to even byte boundaries
        int paddedSize = chunkSize + (chunkSize % 2);

        if (memcmp (id, "clm ", 4) == 0 && chunkSize >= 7)
        {
            // Surge/Serum format: "<!>2048" (or other power-of-2 size)
            juce::MemoryBlock data ((size_t) chunkSize);
            stream.read (data.getData(), chunkSize);
            const char* d = static_cast<const char*> (data.getData());
            if (d[0] == '<' && d[1] == '!' && d[2] == '>')
            {
                // Parse ASCII number after "<!>"
                int sz = std::atoi (d + 3);
                if (sz >= 2 && sz <= 4096)
                    return sz;
            }
        }
        else if (memcmp (id, "smpl", 4) == 0 && chunkSize >= 36)
        {
            // Standard SMPL chunk: 9 × uint32 header, then loop records
            juce::MemoryBlock data ((size_t) chunkSize);
            stream.read (data.getData(), chunkSize);
            const uint32_t* w = static_cast<const uint32_t*> (data.getData());
            uint32_t nloops = w[7];
            if (nloops == 0)
                return 2048; // convention used by some tools
            if (nloops >= 1 && chunkSize >= 36 + 24)
            {
                // Loop record starts at offset 36; fields [2]=start [3]=end
                const uint32_t* lp = w + 9;
                int sz = (int)(lp[3] - lp[2] + 1);
                if (sz >= 2 && sz <= 4096) return sz;
            }
        }
        else
        {
            stream.skipNextBytes (paddedSize);
        }
    }
    return 0; // no metadata found
}

bool WavetableOscillator::loadFromFile (const juce::File& file)
{
    juce::AudioFormatManager mgr;
    mgr.registerFormat (new juce::WavAudioFormat(), true);

    std::unique_ptr<juce::AudioFormatReader> reader (mgr.createReaderFor (file));
    if (reader == nullptr || reader->lengthInSamples < 2)
        return false;

    const auto numSamplesToRead = juce::jmin (reader->lengthInSamples, kMaxWavReadSamples);
    juce::AudioBuffer<float> tempBuffer (1, (int) numSamplesToRead);
    reader->read (&tempBuffer, 0, (int) numSamplesToRead, 0, true, false);

    const float* src    = tempBuffer.getReadPointer (0);
    const int    srcLen = (int) numSamplesToRead;

    // Determine per-frame sample count
    int frameSize = detectWAVFrameSize (file);
    if (frameSize <= 0 && srcLen % kTableSize == 0)
        frameSize = kTableSize;   // heuristic fallback

    const int nFrames = (frameSize > 0) ? juce::jlimit (1, 512, srcLen / frameSize) : 1;

    // Build frames
    std::vector<std::vector<float>> newFrames ((size_t) nFrames,
                                               std::vector<float> ((size_t) kTableSize + 1));

    if (nFrames == 1)
    {
        // Original single-frame path: resample entire file to kTableSize
        for (int i = 0; i < kTableSize; ++i)
        {
            double pos  = (double) i * (srcLen - 1) / kTableSize;
            int    idx  = (int) pos;
            double frac = pos - idx;
            newFrames[0][(size_t) i] = (float) ((1.0 - frac) * src[idx] + frac * src[idx + 1]);
        }
        newFrames[0][(size_t) kTableSize] = newFrames[0][0];
    }
    else
    {
        // Multi-frame path: resample each frameSize-sample chunk → kTableSize
        for (int t = 0; t < nFrames; ++t)
        {
            const float* frameSrc = src + t * frameSize;
            for (int i = 0; i < kTableSize; ++i)
            {
                double pos  = (double) i * (frameSize - 1) / kTableSize;
                int    idx  = (int) pos;
                double frac = pos - idx;
                newFrames[(size_t) t][(size_t) i] =
                    (float) ((1.0 - frac) * frameSrc[idx] + frac * frameSrc[idx + 1]);
            }
            newFrames[(size_t) t][(size_t) kTableSize] = newFrames[(size_t) t][0];
        }
    }

    juce::SpinLock::ScopedLockType sl (tableLock);
    frames    = std::move (newFrames);
    numFrames = nFrames;
    currentWaveform = WaveformType::Custom;
    return true;
}

bool WavetableOscillator::loadWTFile (const juce::File& file)
{
    juce::FileInputStream stream (file);
    if (! stream.openedOk())
        return false;

    // 12-byte header
    char tag[4] = {};
    stream.read (tag, 4);
    if (tag[0] != 'v' || tag[1] != 'a' || tag[2] != 'w' || tag[3] != 't')
        return false;

    uint32_t nSamples = 0;
    uint16_t nTables  = 0;
    uint16_t flags    = 0;
    stream.read (&nSamples, 4);
    stream.read (&nTables,  2);
    stream.read (&flags,    2);

    if (nSamples < 2 || nSamples > 4096 || nTables < 1 || nTables > 512)
        return false;

    const bool isInt16 = (flags & 0x04) != 0;
    const float scale  = 1.0f / 32768.0f;

    std::vector<std::vector<float>> newFrames ((size_t) nTables,
                                               std::vector<float> ((size_t) kTableSize + 1));

    for (int t = 0; t < (int) nTables; ++t)
    {
        std::vector<float> rawFrame ((size_t) nSamples);
        if (isInt16)
        {
            for (int s = 0; s < (int) nSamples; ++s)
            {
                int16_t v = 0;
                stream.read (&v, 2);
                rawFrame[(size_t) s] = (float) v * scale;
            }
        }
        else
        {
            stream.read (rawFrame.data(), (int) (nSamples * sizeof (float)));
        }

        // Resample to kTableSize via linear interpolation
        for (int i = 0; i < kTableSize; ++i)
        {
            double pos  = (double) i * ((int) nSamples - 1) / kTableSize;
            int    idx  = (int) pos;
            double frac = pos - idx;
            newFrames[(size_t) t][(size_t) i] =
                (float) ((1.0 - frac) * rawFrame[(size_t) idx]
                       + frac         * rawFrame[(size_t) (idx + 1)]);
        }
        newFrames[(size_t) t][(size_t) kTableSize] = newFrames[(size_t) t][0];
    }

    juce::SpinLock::ScopedLockType sl (tableLock);
    frames    = std::move (newFrames);
    numFrames = (int) nTables;
    currentWaveform = WaveformType::Custom;
    return true;
}

//==============================================================================
void WavetableOscillator::setMorph (float pos)
{
    morphPosition = juce::jlimit (0.0f, 1.0f, pos);
}

//==============================================================================
void WavetableOscillator::setFrequency (double freq, double sampleRate)
{
    currentBaseFreq   = freq;
    currentSampleRate = sampleRate;
    recomputeVoiceIncrements();
}

void WavetableOscillator::recomputeVoiceIncrements()
{
    if (currentBaseFreq <= 0.0) return;
    for (int v = 0; v < unisonVoices; ++v)
    {
        double normPos = (unisonVoices > 1)
            ? (2.0 * v / (unisonVoices - 1) - 1.0) : 0.0; // [-1, 1]
        double offsetCents = normPos * unisonDetuneCents * unisonBlend;
        double freqMul = std::pow (2.0, offsetCents / kCentsPerOctave);
        phaseIncrement[v] = kTableSize * currentBaseFreq * freqMul / currentSampleRate;
    }
}

void WavetableOscillator::setUnisonParams (int voices, float detuneCents, float blend)
{
    int v = juce::jlimit (1, kMaxVoices, voices);
    if (v == unisonVoices
        && juce::exactlyEqual (detuneCents, unisonDetuneCents)
        && juce::exactlyEqual (blend, unisonBlend))
        return;
    unisonVoices      = v;
    unisonDetuneCents = detuneCents;
    unisonBlend       = blend;
    recomputeVoiceIncrements();
}

//==============================================================================
// Helper: read one interpolated sample from a frame at a given phase position,
// blended across two frames by morphFrac.
static inline float readMorphedSample (const std::vector<std::vector<float>>& frames,
                                       int frameA, int frameB, float frameFrac,
                                       double phase)
{
    auto   i    = (size_t) phase;
    double frac = phase - (double) i;
    float  sA   = (float) ((1.0 - frac) * frames[(size_t) frameA][i]
                          + frac         * frames[(size_t) frameA][i + 1]);
    float  sB   = (float) ((1.0 - frac) * frames[(size_t) frameB][i]
                          + frac         * frames[(size_t) frameB][i + 1]);
    return sA + frameFrac * (sB - sA);
}

float WavetableOscillator::getNextSample()
{
    juce::SpinLock::ScopedTryLockType sl (tableLock);
    if (! sl.isLocked())
        return 0.0f;

    const float morphPos  = morphPosition * (float) (numFrames - 1);
    const int   frameA    = (int) morphPos;
    const int   frameB    = juce::jmin (frameA + 1, numFrames - 1);
    const float frameFrac = morphPos - (float) frameA;

    float s = readMorphedSample (frames, frameA, frameB, frameFrac, phaseIndex[0]);

    phaseIndex[0] += phaseIncrement[0];
    if (phaseIndex[0] >= kTableSize)
        phaseIndex[0] -= kTableSize;

    return s;
}

float WavetableOscillator::getNextSampleUnison()
{
    juce::SpinLock::ScopedTryLockType sl (tableLock);
    if (! sl.isLocked())
        return 0.0f;

    const float morphPos  = morphPosition * (float) (numFrames - 1);
    const int   frameA    = (int) morphPos;
    const int   frameB    = juce::jmin (frameA + 1, numFrames - 1);
    const float frameFrac = morphPos - (float) frameA;

    float sum = 0.0f;
    for (int v = 0; v < unisonVoices; ++v)
    {
        sum += readMorphedSample (frames, frameA, frameB, frameFrac, phaseIndex[v]);
        phaseIndex[v] += phaseIncrement[v];
        if (phaseIndex[v] >= kTableSize) phaseIndex[v] -= kTableSize;
    }
    return sum / (float) unisonVoices;
}

void WavetableOscillator::reset()
{
    for (int v = 0; v < kMaxVoices; ++v)
    {
        phaseIndex[v]      = (double) v / kMaxVoices * kTableSize;
        phaseIncrement[v]  = 0.0;
    }
    currentBaseFreq = 0.0;
}

std::vector<float> WavetableOscillator::getFrameForDisplay (float morphPos) const
{
    juce::SpinLock::ScopedTryLockType sl (tableLock);
    if (! sl.isLocked() || frames.empty()) return {};

    const float mp   = juce::jlimit (0.0f, 1.0f, morphPos) * (float) (numFrames - 1);
    const int   fa   = (int) mp;
    const int   fb   = juce::jmin (fa + 1, numFrames - 1);
    const float frac = mp - (float) fa;

    std::vector<float> result ((size_t) kTableSize);
    for (int i = 0; i < kTableSize; ++i)
        result[(size_t) i] = frames[(size_t) fa][(size_t) i]
                           + frac * (frames[(size_t) fb][(size_t) i]
                                   - frames[(size_t) fa][(size_t) i]);
    return result;
}
