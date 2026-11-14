# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**JQ Gunk** is a JUCE-based audio plugin (VST3 + Standalone) for bass synthesis and processing. It detects the pitch of an incoming bass signal and synthesizes oscillator tones at that frequency, with an envelope-controlled resonant filter, LFO, modulation matrix, transient player, and impulse response convolution.

## Build Commands

```bash
# Configure (RelWithDebInfo with ENABLE_DEBUG_LOG)
cmake --preset build

# Configure (Release with LTO)
cmake --preset release

# Configure (Windows cross-compile via mingw-w64)
cmake --preset windows

# Build
cmake --build build
cmake --build build-release

# Install factory presets to ~/.config/JQGunk/Factory Presets/
cmake --build build --target install-presets

# Install factory wavetables to ~/.config/JQGunk/Wavetables/
cmake --build build --target install-wavetables

# Install factory impulse responses to ~/.config/JQGunk/Impulse Responses/
cmake --build build --target install-irs
```

Build artifacts land in `build/JQGunk_artefacts/` (VST3 + Standalone).

There are no automated tests. Verification is done by loading the plugin in a DAW or running the Standalone app.

## External Dependencies (fetched via CMake FetchContent)

- **JUCE 7.0.12** — Audio plugin framework
- **Cycfi Q** — Envelope follower (`ar_envelope_follower`) and noise gate (`noise_gate`) used in PluginProcessor
- **Aubio (fork)** — Onset/transient detection via `AubioOnsetDetector`. Uses `github.com/cemkod/aubio` (pinned commit) instead of upstream 0.4.9; fork adds a skip-phas-norm-only-odfs optimisation (PR pending upstream).

## Architecture

Signal flow: **Audio Input → Pitch Detector → Oscillators (OSC1 + OSC2 + Sub, each with unison) → Envelope Gate → Drive (2x oversampled) → Resonant Filter → Transient Player → IR Convolution → Mix → Output**

The dry input normally skips drive and filter and joins at the mix. With `dryRouting` = Through FX, it is added to the synth before Drive, and the synth's amp envelope moves ahead of Drive so the dry signal isn't enveloped.

Stereo: with a stereo output bus, each oscillator also produces a side signal (unison voices panned by their detune position × `unisonWidth`, linear pan law so (L+R)/2 equals the mono signal). The summed side is high-passed at `stereoMonoBelow` (`SideHighpass`), then L = mid + side and R = mid − side go through Drive, two filter instances, the mix and a two-channel IR. Dry, sub and transient are centred. A mono output bus takes the original mono path.

Modulation: **LFO + Envelope + Mod Envelope + Pitch → Mod Matrix (8 slots, 21 targets) → per-block parameter offsets**

### Key Source Files

**DSP Engine:**
- `Source/PluginProcessor.h/.cpp` — Core audio engine. Owns APVTS, all DSP modules. Orchestrates block-based processing.
- `Source/PitchDetector.h/.cpp` — `AutocorrelationPitchDetector`: McLeod Pitch Method, 40–400 Hz, 8192-sample buffer with 128-sample hop.
- `Source/PitchDetectorThread.h` — Runs pitch detection on a background thread.
- `Source/Oscillator.h/.cpp` — `WavetableOscillator`: 2048-point wavetable, 4 built-in waveforms (Sine/Triangle/Square/Sawtooth), custom WAV/WT loading, SpinLock for thread-safe wavetable swapping. Supports unison via `setUnisonParams()`.
- `Source/Filter.h` / `Source/FilterEngine.h` — `EnvelopeFilter`: SVF-based resonant filter with envelope-controlled sweep.
- `Source/DriveEngine.h` — `DriveEngine`: pre-filter waveshaper (Soft/Hard/Tube) at 2x via `juce::dsp::Oversampling` (polyphase IIR), with auto-gain and a DC blocker. Bypassed when `driveEnabled` is off.
- `Source/SideHighpass.h` — 12 dB/oct TPT high-pass on the side signal (the MONO < control).
- `Source/GlideEngine.h` — Pitch portamento/glide.
- `Source/LFOEngine.h` — Low-frequency oscillator (mod source).
- `Source/ModMatrix.h` — Modulation routing: 8 slots × 5 sources (None/Envelope/Pitch/ModEnvelope/LFO) × 21 targets. Presets store the target index, so new `ModTarget` values must be appended. Per-target scaling in `kTargetScale[]`. Call `snapshot()` then `computeTargetBlock()` per audio block.
- `Source/AubioOnsetDetector.h` — Onset/transient detection wrapping aubio.
- `Source/TransientPlayer.h` — Sample playback triggered by onsets, with pitch tracking.
- `Source/IRConvolutionEngine.h` — `IRConvolutionEngine`: wraps `juce::dsp::Convolution` for dry/wet impulse response processing. Factory IRs in `~/.config/JQGunk/Impulse Responses/`, shipped in `ImpulseResponses/Factory/`.
- `Source/ParamCache.h` — `ParamCache`: holds raw `std::atomic<float>*` pointers to all frequently-read APVTS parameters. Initialized once in the processor constructor via `params.init(apvts)`; avoids repeated APVTS string lookups on the audio thread.
- `Source/PresetManager.h/.cpp` — Preset file I/O. Factory presets in `~/.config/JQGunk/Factory Presets/`, user presets in `~/.config/JQGunk/User Presets/`. Files are `*.jqgpreset` (XML). Call `syncOscillatorAfterPresetLoad()` after loading.
- `Source/ParameterIDs.h` — All APVTS parameter ID string constants in `ParamIDs` namespace.

**UI:**
- `Source/PluginEditor.h/.cpp` — Main editor; delegates to section components. Timer-driven live frequency label.
- `Source/LookAndFeel.h` — `BassLookAndFeel`: dark navy/purple theme with yellow accent.
- Section components follow naming pattern `*SectionComponent.h` (Osc, CombinedOsc, Drive, Filter, Gate, Envelope, LFO, Transient, IR, Output, ModMatrix).
- Display components follow pattern `*DisplayComponent.h` (Filter, Gate, Envelope, LFO, Waveform).
- `Source/PowerToggleButton.h` — `PowerToggleButton`: themed power-icon toggle button used to enable/disable each section (OSC1, OSC2, Sub, Filter, Transient, IR).

### Notable Implementation Details

- Plugin is an **audio effect** (not a synth): `IS_SYNTH FALSE`, no MIDI.
- Two oscillators (OSC1 + OSC2) plus a sub oscillator, each configurable independently.
- `ENABLE_DEBUG_LOG` is defined only in the `build` (RelWithDebInfo) preset.
- Logo SVG is embedded as binary data under the `JQGunkData` namespace via `juce_add_binary_data`.
- APVTS state ID: `"Parameters"` — root XML tag is `<Parameters>`.
- `waveform` choice indices 0/1/2 map to Triangle/Square/Sawtooth (add 1 to get `WaveformType` enum value; Sine=0 is not exposed in the choice).
- Factory presets ship in `Presets/Factory/`, wavetables in `Wavetables/` (includes `Wavetables/Gunk/` for custom wavetables; Surge XT-sourced tables are GPL v3), and impulse responses in `ImpulseResponses/Factory/`.
- Each major section (OSC1, OSC2, Sub, Drive, Filter, Transient, IR) has an `*Enabled` boolean parameter that bypasses that section's processing entirely.
- C++20 standard.
