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

    void trigger (float attackSec, float decaySec)
    {
        playhead       = 0;
        attackSamples  = (int) (currentSampleRate * attackSec);
        attackCoeff    = 1.0f - std::exp (-1.0f / (float) (currentSampleRate * attackSec));
        decayCoeff     = 1.0f - std::exp (-1.0f / (float) (currentSampleRate * decaySec));
        envValue       = 0.0f;
        envPhaseCounter = 0;
        envPhase       = EnvPhase::Attack;
        isPlaying      = true;
    }

    float getNextSample()
    {
        if (! isPlaying) return 0.0f;

        juce::SpinLock::ScopedTryLockType sl (sampleLock);
        if (! sl.isLocked()) return 0.0f;

        if (sampleData.empty() || playhead >= (int) sampleData.size())
        {
            isPlaying = false;
            return 0.0f;
        }

        // Advance envelope
        if (envPhase == EnvPhase::Attack)
        {
            envValue += attackCoeff * (1.0f - envValue);
            ++envPhaseCounter;
            if (envPhaseCounter >= attackSamples)
            {
                envValue  = 1.0f;
                envPhase  = EnvPhase::Decay;
            }
        }
        else if (envPhase == EnvPhase::Decay)
        {
            envValue += decayCoeff * (0.0f - envValue);
        }

        const float out = sampleData[(size_t) playhead] * envValue;
        ++playhead;
        return out;
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

    bool isSampleLoaded() const { return sampleLoaded.load (std::memory_order_acquire); }

    juce::String getLoadedFilePath() const
    {
        juce::SpinLock::ScopedLockType sl (sampleLock);
        return loadedFilePath;
    }

    void reset()
    {
        isPlaying       = false;
        playhead        = 0;
        envValue        = 0.0f;
        envPhase        = EnvPhase::Idle;
        envPhaseCounter = 0;
        attackSamples   = 0;
        attackCoeff     = 0.0f;
        decayCoeff      = 0.0f;
    }

private:
    mutable juce::SpinLock sampleLock;
    std::vector<float> sampleData;
    std::atomic<bool>  sampleLoaded { false };
    juce::String       loadedFilePath;

    // Audio-thread only state (no lock needed)
    int   playhead        = 0;
    bool  isPlaying       = false;
    double currentSampleRate = 48000.0;
    float envValue        = 0.0f;
    float attackCoeff     = 0.0f;
    float decayCoeff      = 0.0f;
    int   attackSamples   = 0;
    int   envPhaseCounter = 0;

    enum class EnvPhase { Idle, Attack, Decay } envPhase = EnvPhase::Idle;
};
