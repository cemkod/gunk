#pragma once

#include <JuceHeader.h>

class TransientPlayer
{
public:
    void prepare (double sampleRate)
    {
        currentSampleRate = sampleRate;
        reset();
    }

    void trigger (float attackSec, float holdSec, float decaySec)
    {
        playheadFrac = 0.0;
        attackCoeff  = 1.0f - std::exp (-1.0f / std::max (1.0f, (float) currentSampleRate * attackSec));
        holdSamples  = (int) (currentSampleRate * holdSec);
        decayCoeff   = 1.0f - std::exp (-1.0f / std::max (1.0f, (float) currentSampleRate * decaySec));
        // Keep current envValue so retriggers don't jump to silence
        holdCounter  = 0;
        envPhase     = EnvPhase::Attack;
        isPlaying    = true;
    }

    // rate = playback speed ratio (1.0 = original pitch, 2.0 = +12 st, 0.5 = -12 st)
    float getNextSample (float rate = 1.0f)
    {
        if (! isPlaying) return 0.0f;

        // Advance envelope — must happen regardless of sample data
        if (envPhase == EnvPhase::Attack)
        {
            envValue += attackCoeff * (1.0f - envValue);
            if (envValue >= 0.999f)
            {
                envValue = 1.0f;
                envPhase = (holdSamples > 0) ? EnvPhase::Hold : EnvPhase::Decay;
            }
        }
        else if (envPhase == EnvPhase::Hold)
        {
            if (++holdCounter >= holdSamples)
                envPhase = EnvPhase::Decay;
        }
        else if (envPhase == EnvPhase::Decay)
        {
            envValue += decayCoeff * (0.0f - envValue);
            if (envValue < 1e-4f)
            {
                envValue  = 0.0f;
                isPlaying = false;
            }
        }

        // Sample playback (returns 0 if no sample loaded or exhausted)
        juce::SpinLock::ScopedTryLockType sl (sampleLock);
        if (! sl.isLocked()) return 0.0f;

        const int size = (int) sampleData.size();
        if (size < 2 || playheadFrac >= (double) size)
            return 0.0f;

        const int   i0   = (int) playheadFrac;
        const int   i1   = juce::jmin (i0 + 1, size - 1);
        const float frac = (float) (playheadFrac - (double) i0);
        const float s    = sampleData[(size_t) i0] + frac * (sampleData[(size_t) i1] - sampleData[(size_t) i0]);

        playheadFrac += (double) rate;
        return s * envValue;
    }

    // Fills sampleOut and envOut (envOut may be nullptr) with N samples at the given rate.
    // envOut captures envValue before each sample's envelope advance.
    void processBlock (float* sampleOut, float* envOut, int n, float rate)
    {
        for (int i = 0; i < n; ++i)
        {
            if (envOut) envOut[i] = envValue;
            sampleOut[i] = getNextSample (rate);
        }
    }

    bool loadFromFile (const juce::File& file)
    {
        juce::AudioFormatManager mgr;
        mgr.registerFormat (new juce::WavAudioFormat(), true);
        mgr.registerFormat (new juce::AiffAudioFormat(), true);

        std::unique_ptr<juce::AudioFormatReader> reader (mgr.createReaderFor (file));
        if (reader == nullptr || reader->lengthInSamples < 2)
            return false;

        // Cap at 10 seconds
        const juce::int64 maxSamples = (juce::int64) (reader->sampleRate * 10.0);
        const int numToRead = (int) juce::jmin (reader->lengthInSamples, maxSamples);

        const int numChannels = (int) reader->numChannels;
        juce::AudioBuffer<float> tempBuffer (numChannels, numToRead);
        reader->read (&tempBuffer, 0, numToRead, 0, true, true);

        // Mix to mono
        std::vector<float> newData ((size_t) numToRead);
        for (int i = 0; i < numToRead; ++i)
        {
            float sum = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch)
                sum += tempBuffer.getReadPointer (ch)[i];
            newData[(size_t) i] = sum / (float) numChannels;
        }

        {
            juce::SpinLock::ScopedLockType sl (sampleLock);
            std::swap (sampleData, newData);
            loadedFilePath = file.getFullPathName();
            sampleLoaded.store (true, std::memory_order_release);
        }
        return true;
    }

    float getEnvelopeValue() const noexcept { return envValue; }

    bool isSampleLoaded() const { return sampleLoaded.load (std::memory_order_acquire); }

    juce::String getLoadedFilePath() const
    {
        juce::SpinLock::ScopedLockType sl (sampleLock);
        return loadedFilePath;
    }

    void reset()
    {
        isPlaying    = false;
        playheadFrac = 0.0;
        envValue     = 0.0f;
        envPhase     = EnvPhase::Idle;
        holdCounter  = 0;
        holdSamples  = 0;
        attackCoeff  = 0.0f;
        decayCoeff   = 0.0f;
    }

private:
    mutable juce::SpinLock sampleLock;
    std::vector<float> sampleData;
    std::atomic<bool>  sampleLoaded { false };
    juce::String       loadedFilePath;

    // Audio-thread only state (no lock needed)
    double playheadFrac      = 0.0;
    bool   isPlaying         = false;
    double currentSampleRate = 48000.0;
    float  envValue          = 0.0f;
    float  attackCoeff       = 0.0f;
    float  decayCoeff        = 0.0f;
    int    holdSamples       = 0;
    int    holdCounter       = 0;

    enum class EnvPhase { Idle, Attack, Hold, Decay } envPhase = EnvPhase::Idle;
};
