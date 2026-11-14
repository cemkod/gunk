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
    void prepare (double sampleRate, int maxBlockSize)
    {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate       = sampleRate;
        spec.maximumBlockSize = (juce::uint32) maxBlockSize;
        spec.numChannels      = 1;
        convolution.prepare (spec);
        prepared = true;

        wetBuffer.setSize (1, maxBlockSize);
        wetBuffer.clear();
    }

    // Safe to call before prepare(); the IR becomes active once prepared.
    // Loading itself happens on JUCE's background thread.
    void loadIR (const juce::File& file)
    {
        convolution.loadImpulseResponse (
            file,
            juce::dsp::Convolution::Stereo::no,
            juce::dsp::Convolution::Trim::yes,
            0,
            juce::dsp::Convolution::Normalise::no);
        loadedPath = file.getFullPathName();
        irLoaded   = true;
    }

    // In-place dry/wet blend. mix=0 → dry only, mix=1 → wet only.
    // No-op if no IR has been loaded, prepare() not called, or mix is 0.
    void processBlock (float* data, int numSamples, float mix, float makeup = 1.0f)
    {
        if (! irLoaded || ! prepared || mix <= 0.0f)
            return;

        // Copy dry signal into wetBuffer, then convolve in-place
        wetBuffer.copyFrom (0, 0, data, numSamples);

        juce::dsp::AudioBlock<float> block (
            wetBuffer.getArrayOfWritePointers(), 1, (size_t) numSamples);
        convolution.process (juce::dsp::ProcessContextReplacing<float> (block));

        const float  dryGain = 1.0f - mix;
        const float  wetGain = mix * makeup;
        const float* wet     = wetBuffer.getReadPointer (0);
        for (int i = 0; i < numSamples; ++i)
            data[i] = data[i] * dryGain + wet[i] * wetGain;
    }

    bool          isLoaded()      const { return irLoaded; }
    juce::String  getLoadedPath() const { return loadedPath; }

private:
    juce::dsp::Convolution   convolution;
    juce::AudioBuffer<float> wetBuffer;
    juce::String             loadedPath;
    bool                     irLoaded = false;
    bool                     prepared = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IRConvolutionEngine)
};
