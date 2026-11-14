# JQ Gunk review: state of the plugin and what to improve

As of 2026-11-14

## Summary

JQ Gunk is a well-built plugin with a clear identity, but its sound engine is plainer than the name promises and one state bug loses the user's impulse response on session reload.

The review covers v0.3.0 plus the uncommitted work on the `ADSR` branch. Recommended order of work:

1. Fix the impulse-response reload bug and the ADSR trigger. Both are small and the ADSR work is already in progress.
2. Add a drive stage and stereo unison. These are the largest gains in sound for the least work.
3. Reset the pitch window on each detected onset. This targets wrong-note blips at note changes.
4. Rebuild the factory presets. The existing ten predate the mod matrix and reference parameters that no longer exist.

## What works well

The real-time architecture is sound, which is the hard part of a pitch-tracking effect.

- **Pitch detection runs off the audio thread.** `PitchDetectorThread` takes samples through a lock-free FIFO and publishes the result through atomics.
- **Processing is block-based.** `processBlock` fills per-sample scratch buffers in tight loops, with no allocation in the steady state.
- **Parameters are cached.** `ParamCache` and `ModMatrix::init` hold raw atomic pointers, so the audio thread does no string lookups.
- **The pitch detector is careful.** It uses FFT-accelerated McLeod with onset and sustain hysteresis, a clarity gate, and sub-sample lag interpolation.
- **The modular design is the identity.** Eight mod slots over envelope, pitch, mod envelope and LFO give the plugin more range than a fixed auto-wah.

## Bugs to fix first

Six defects were found by reading the code; the impulse-response one is the most serious because it silently loses user data.

| Bug | Where | Effect | Fix |
| --- | --- | --- | --- |
| IR is lost on session reload | `IRConvolutionEngine::loadIR`, `getStateInformation` | `loadIR` returns early when `prepare()` has not run, and hosts often restore state first. The next save then drops `irSamplePath` because `isLoaded()` is false. | Store the path and load it in `prepare()` |
| IR is lost on re-prepare | `IRConvolutionEngine::prepare` | Each call builds a new `juce::dsp::Convolution`, so a sample-rate or buffer-size change discards the loaded IR. | Same fix: reload the stored path in `prepare()` |
| ADSR only triggers on an aubio onset | `PluginProcessor.cpp:588` | A hammer-on or soft legato note that the onset detector misses is silent in Env Module mode. | Also trigger on the gate-open edge |
| ADSR velocity steps during the attack | `ADSREnvelope::trackVelocity` | Velocity is a raw peak that jumps for 100 ms and multiplies the envelope directly, so the attack is stepped. | Smooth the velocity, or latch it once |
| `setWaveform` runs on the audio thread | `updateOscParams` | It resizes vectors, which can free memory when switching from a multi-frame wavetable to a basic shape. | Build the table off-thread and swap it in |
| Two text-entry ranges are wrong | `createParameterLayout` | `transientDecay` clamps typed values to 0.05 on a 0.2 range; `transientDry` allows 0–2 on a 0–1 range. | Match the parse limits to the ranges |

Two smaller portability issues sit beside these. Transient-sample and IR paths are saved as absolute paths, so presets that use them do not move between machines. `getTailLengthSeconds` returns 0 although the IR and the release envelope both ring out.

## Sound engine

The engine has no nonlinearity, no stereo and a 4 kHz filter ceiling, so it sounds cleaner and narrower than a plugin called Gunk should.

| Gap | Where | Suggested change |
| --- | --- | --- |
| No drive or saturation anywhere in the chain | `processBlock` signal path | Add a drive stage before the filter, with oversampling |
| Output is mono, copied to every channel | Final write loop in `PluginProcessor.cpp` | Pan unison voices across the stereo field |
| Chamberlin SVF, cutoff capped at 4 kHz, cutoff and modulation in linear Hz | `Filter.h`, `kTargetScale` in `ModMatrix.h` | Use a TPT/ZDF SVF up to 20 kHz and modulate in octaves |
| Wavetables are read naively, with no band-limited mipmaps | `WavetableOscillator::processBlock` | Add mipmapped tables, so saw and square do not alias when shifted up |
| Only filter cutoff is modulated per sample; levels, volume and resonance step once per block | `processBlock` step 4 | Smooth block-rate values or compute them per sample |

Drive and stereo come first because each is a contained change with an obvious audible result. The filter and mipmap work is larger and changes how existing presets sound.

## Pitch tracking

Tracking is the product, and the plugin already contains the piece that would improve it most: the onset detector.

- **Reset the analysis window on each onset.** The detector analyses a fixed 4096-sample window, so after a note change it still holds the old note. Starting a fresh, growing window at the onset lets the new pitch be read sooner and reduces wrong-note blips.
- **Make the window independent of sample rate.** The window is a fixed sample count. At 96 kHz, 4096 samples is about 43 ms, roughly 1.3 periods of a low B at 31 Hz, which is too short for reliable detection.
- **Decimate before analysis.** The input is already low-passed at 500 Hz. Downsampling by 4 fixes the sample-rate problem and shrinks the 8192-point FFT.

The 500 Hz pre-filter is a single second-order section, so decimation needs a steeper anti-alias filter in front of it.

## Product

The plugin has no regression tests for its core feature and ships presets that no longer match the engine.

- **Tracking tests.** There are no automated tests. An offline harness that runs recorded bass DI files through the detector and reports pitch error and latency would let thresholds be tuned with evidence. `pitch-debugger/` is a possible starting point.
- **Factory presets.** The ten existing presets set 13 parameters each, use no mod routing, and reference `mix` and `envDecay`, which are no longer parameters. Six new presets were added alongside this review: Acid Squelch, Talkbox, Reese Engine, Wobble Step, Glass Pluck and Bowed Fretless. They are validated against the parameter list but not yet auditioned.
- **Plugin formats.** The build produces VST3 and Standalone only. JUCE 7 also supports LV2 and AU, and a macOS signing setup already exists.
- **LFO.** It is unipolar, with no tempo sync and no retrigger on onset.

Open question: some saved user presets contain parameters with no value, such as `<PARAM id="morph"/>` in `Thriller.jqgpreset`. If loading leaves those at their previous value, a preset sounds different depending on what was loaded before it. This was not confirmed against the JUCE source.

## Scope of the review

Every finding comes from reading the source; nothing was built, run or listened to.

- **Read:** the processor, pitch detector and its thread, oscillators, filter, glide, LFO, mod matrix, transient player, IR engine, onset detector, the new `ADSREnvelope.h`, the preset manager and the factory presets.
- **Not reviewed:** the editor and section components, the build and packaging scripts, and the aubio fork.
- **Not measured:** CPU use, tracking latency and tracking accuracy. The tracking recommendations follow from the window sizes in the code, not from recordings.
