#pragma once
#include <JuceHeader.h>
#include "PitchDetector.h"
#include "DebugLog.h"

class PitchDetectorThread : public juce::Thread
{
public:
    static constexpr int kFifoSize = 8192;

    PitchDetectorThread() : juce::Thread ("PitchDetectorThread"), fifo (kFifoSize) {}
    ~PitchDetectorThread() override { stop(); }

    // Call from prepareToPlay (message or audio thread). Safe to call multiple times.
    void prepare (double sampleRate)
    {
        pendingSampleRate.store (sampleRate, std::memory_order_relaxed);
        sampleRateChanged.store (true, std::memory_order_relaxed);
        wakeUp.signal();
        if (!isThreadRunning())
            startThread (juce::Thread::Priority::high);
    }

    void stop()
    {
        signalThreadShouldExit();
        wakeUp.signal();
        stopThread (1000);
    }

    // Audio-thread API
    void pushSamples (const float* data, int numSamples)
    {
        int s1, n1, s2, n2;
        fifo.prepareToWrite (numSamples, s1, n1, s2, n2);
        if (n1 > 0) juce::FloatVectorOperations::copy (fifoBuffer + s1, data,      n1);
        if (n2 > 0) juce::FloatVectorOperations::copy (fifoBuffer + s2, data + n1, n2);
        fifo.finishedWrite (n1 + n2);
        if (++blockCount >= 2)
        {
            blockCount = 0;
            wakeUp.signal();
        }
    }

    void setPendingWindowSize (int w)
    {
        pendingWindowSize.store (w, std::memory_order_relaxed);
    }

    float getDetectedFrequency() const
    {
        return detectedFrequency.load (std::memory_order_relaxed);
    }

private:
    void run() override
    {
        applyPendingConfig();
        while (!threadShouldExit())
        {
            wakeUp.wait (-1);
            if (threadShouldExit()) break;
            applyPendingConfig();
            drainFifo();
        }
    }

    void applyPendingConfig()
    {
        if (sampleRateChanged.exchange (false, std::memory_order_relaxed))
        {
            const double sr = pendingSampleRate.load (std::memory_order_relaxed);
            detector.reset();
            detector.setSampleRate (sr);
        }
        const int w = pendingWindowSize.exchange (0, std::memory_order_relaxed);
        if (w > 0)
            detector.setWindowSize (w);
    }

    void drainFifo()
    {
        const int numReady = fifo.getNumReady();
        if (numReady == 0) return;

#ifdef ENABLE_DEBUG_LOG
        const juce::int64 t0 = juce::Time::getHighResolutionTicks();
#endif

        // Read FIFO segments and feed directly to detector
        int s1, n1, s2, n2;
        fifo.prepareToRead (numReady, s1, n1, s2, n2);
        if (n1 > 0) detector.processBlock (fifoBuffer + s1, n1);
        if (n2 > 0) detector.processBlock (fifoBuffer + s2, n2);
        fifo.finishedRead (n1 + n2);

        const float freq = detector.getFrequency();
        detectedFrequency.store (freq, std::memory_order_relaxed);

#ifdef ENABLE_DEBUG_LOG
        const juce::int64 t1 = juce::Time::getHighResolutionTicks();
        const double totalMs = juce::Time::highResolutionTicksToSeconds (t1 - t0) * 1000.0;
        if (totalMs > ptMaxMs) ptMaxMs = totalMs;
        ptTotalMs += totalMs;
        ++ptCount;
        if (ptCount >= 500)
        {
            dbgLog ("PITCH-THREAD samples=" + juce::String (numReady)
                    + " | avg=" + juce::String (ptTotalMs / ptCount, 3) + "ms"
                    "  max=" + juce::String (ptMaxMs, 3) + "ms"
                    " | freq=" + juce::String (freq, 1));
            ptCount = 0; ptTotalMs = 0; ptMaxMs = 0;
        }
#endif
    }

    // Audio sample FIFO (single producer = audio thread, single consumer = pitch thread)
    juce::AbstractFifo fifo;
    float fifoBuffer[kFifoSize] = {};

    // DSP objects — owned exclusively by pitch thread after prepare()
    AutocorrelationPitchDetector detector;

    // Signals: audio thread → pitch thread
    std::atomic<bool>   sampleRateChanged { false };
    std::atomic<double> pendingSampleRate { 44100.0 };
    std::atomic<int>    pendingWindowSize { 0 };

    // Result: pitch thread → audio thread
    std::atomic<float>  detectedFrequency { 0.0f };

    int blockCount = 0; // audio-thread only: wake pitch thread every 2 blocks
    juce::WaitableEvent wakeUp; // auto-reset (default = false)

#ifdef ENABLE_DEBUG_LOG
    int    ptCount = 0;
    double ptTotalMs = 0, ptMaxMs = 0;
#endif
};
