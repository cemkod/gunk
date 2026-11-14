#pragma once

#include <JuceHeader.h>

class IRConvolutionEngine
{
public:
    IRConvolutionEngine() = default;

    // The Convolution object lives for the engine's lifetime. Per the JUCE docs,
    // prepare() fully initialises the IR from the most recent loadImpulseResponse()
    // call, so an IR loaded before prepare() (e.g. during state restore) or before a
    // re-prepare (sample-rate / block-size change) survives.
    //
    // Threading (JUCE docs): "load() calls must be synchronised with process() calls,
    // which in practice means making the load() call from the audio thread." So
    // loadIR() only posts the file; the audio thread calls loadImpulseResponse() in
    // applyPendingLoad(). prepare() also applies it, since the host never runs
    // prepare() concurrently with process().
    void prepare (double newSampleRate, int maxBlockSize)
    {
        applyPendingLoad();

        juce::dsp::ProcessSpec spec;
        spec.sampleRate       = newSampleRate;
        spec.maximumBlockSize = (juce::uint32) maxBlockSize;
        spec.numChannels      = 1;
        convolution.prepare (spec);
        prepared         = true;
        sampleRate       = newSampleRate;
        tailSeconds.store (convolution.getCurrentIRSize() / sampleRate, std::memory_order_relaxed);

        wetBuffer.setSize (1, maxBlockSize);
        wetBuffer.clear();
    }

    // Message thread. Queues the file; the actual load is issued from the audio
    // thread (or the next prepare()). The decode itself runs on JUCE's background thread.
    void loadIR (const juce::File& file)
    {
        {
            const juce::SpinLock::ScopedLockType lock (pendingLock);
            pendingFile = file;
            hasPending  = true;
        }
        loadedPath = file.getFullPathName();
        irLoaded   = true;
    }

    // Audio thread, once per block, regardless of whether the IR section is enabled.
    // Never blocks: if the message thread holds the lock, try again next block.
    void applyPendingLoad()
    {
        const juce::SpinLock::ScopedTryLockType lock (pendingLock);
        if (! lock.isLocked() || ! hasPending)
            return;

        // Copying a File only bumps the path string's refcount, and pendingFile keeps
        // its own reference, so nothing is allocated or freed here.
        convolution.loadImpulseResponse (
            pendingFile,
            juce::dsp::Convolution::Stereo::no,
            juce::dsp::Convolution::Trim::yes,
            0,
            juce::dsp::Convolution::Normalise::no);
        hasPending = false;
        audioHasIR = true;
    }

    // In-place dry/wet blend. mix=0 → dry only, mix=1 → wet only.
    // No-op if no IR has been loaded, prepare() not called, or mix is 0.
    void processBlock (float* data, int numSamples, float mix, float makeup = 1.0f)
    {
        if (! audioHasIR || ! prepared || mix <= 0.0f)
            return;

        // Copy dry signal into wetBuffer, then convolve in-place
        wetBuffer.copyFrom (0, 0, data, numSamples);

        juce::dsp::AudioBlock<float> block (
            wetBuffer.getArrayOfWritePointers(), 1, (size_t) numSamples);
        convolution.process (juce::dsp::ProcessContextReplacing<float> (block));

        // getCurrentIRSize() reads the engine that process() swaps, so it may only be
        // called here; publish it for the host's message-thread tail query.
        tailSeconds.store (convolution.getCurrentIRSize() / sampleRate, std::memory_order_relaxed);

        const float  dryGain = 1.0f - mix;
        const float  wetGain = mix * makeup;
        const float* wet     = wetBuffer.getReadPointer (0);
        for (int i = 0; i < numSamples; ++i)
            data[i] = data[i] * dryGain + wet[i] * wetGain;
    }

    // Any thread. Length of the active IR, as last seen by the audio thread.
    double        getTailSeconds() const { return tailSeconds.load (std::memory_order_relaxed); }
    bool          isLoaded()      const { return irLoaded; }
    juce::String  getLoadedPath() const { return loadedPath; }

private:
    juce::dsp::Convolution   convolution;
    juce::AudioBuffer<float> wetBuffer;
    // Message-thread state (UI / state save).
    juce::String             loadedPath;
    bool                     irLoaded = false;

    // Handoff from message thread to audio thread, guarded by pendingLock.
    juce::SpinLock           pendingLock;
    juce::File               pendingFile;
    bool                     hasPending = false;

    // Audio-thread state.
    bool                     audioHasIR = false;
    bool                     prepared   = false;
    double                   sampleRate = 44100.0;

    std::atomic<double>      tailSeconds { 0.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IRConvolutionEngine)
};
