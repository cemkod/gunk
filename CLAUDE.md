# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**JQ Gunk** is a JUCE-based audio plugin (VST3 + Standalone) for bass synthesis and processing. It detects the pitch of an incoming bass signal and synthesizes a clean oscillator tone at that frequency, with an envelope-controlled resonant filter (auto-wah effect).

## Build Commands

```bash
# Configure (RelWithDebInfo with ENABLE_DEBUG_LOG)
cmake --preset build

# Configure (Release with LTO)
cmake --preset release

# Build
cmake --build build
cmake --build build-release

# Install factory presets to ~/.config/JQGunk/Factory Presets/
cmake --build build --target install-presets
```

Build artifacts land in `build/JQGunk_artefacts/` (VST3 + Standalone).

There are no automated tests. Verification is done by loading the plugin in a DAW or running the Standalone app.

## Architecture

Signal flow: **Audio Input → Pitch Detector → Wavetable Oscillator (+ Unison + Sub) → Envelope Gate → Resonant Filter → Mix → Output**

### Key Files

- **`Source/PluginProcessor.h/.cpp`** — Core audio engine. Owns APVTS, `PresetManager`, `AutocorrelationPitchDetector`, `EnvelopeFilter`, and two `WavetableOscillator` instances (`oscillator` + `subOscillator`). Orchestrates all DSP.
- **`Source/PitchDetector.h`** — `AutocorrelationPitchDetector`: FFT-based McLeod Pitch Method, 40–400 Hz, 8192-sample buffer with 128-sample hop, hysteresis gating.
- **`Source/Oscillator.h/.cpp`** — `WavetableOscillator`: 2048-point wavetable, 4 built-in waveforms (Sine/Triangle/Square/Sawtooth), custom WAV loading, SpinLock for thread-safe wavetable swapping. Supports unison via `setUnisonParams(voices, detuneCents, blend)`.
- **`Source/Filter.h`** / **`Source/FilterEngine.h`** — `EnvelopeFilter`: SVF-based resonant lowpass with envelope-controlled auto-wah sweep.
- **`Source/PresetManager.h/.cpp`** — Preset file I/O. Factory presets in `~/.config/JQGunk/Factory Presets/`, user presets in `~/.config/JQGunk/User Presets/`. Files are `*.jqgpreset` (XML with `<Parameters>` root). Call `syncOscillatorAfterPresetLoad()` after loading a preset.
- **`Source/PluginEditor.h/.cpp`** — Main editor; delegates UI sections to component classes below. Timer-driven live frequency label.
- **`Source/OscSectionComponent.h`** — Oscillator UI (waveform buttons, unison, sub level controls).
- **`Source/FilterSectionComponent.h`** / **`Source/FilterDisplayComponent.h`** — Filter controls and graphical filter curve display.
- **`Source/GateSectionComponent.h`** — Gate threshold/hysteresis controls with VU meter.
- **`Source/LookAndFeel.h`** — `BassLookAndFeel` (extends `LookAndFeel_V4`): dark navy/purple theme with yellow accent (`#fffff100`).

### APVTS Parameters

| ID | Range | Default | Description |
|----|-------|---------|-------------|
| `gateThreshold` | 0.001–0.04 (log dB scale) | 0.01 | Gate open threshold |
| `gateHysteresis` | 0–6 dB | 3.5 | Hysteresis above threshold to keep gate open |
| `glide` | 0–1 s | 0 | Pitch glide time |
| `mix` | 0–1 | 1.0 | Dry/wet blend |
| `waveform` | choice | Triangle | Oscillator waveform (Triangle/Square/Sawtooth) |
| `envSensitivity` | 0–7 | 3.0 | Envelope follower sensitivity |
| `envResonance` | 0–8 | 2.0 | Filter Q |
| `envDecay` | 0.01–2 s | 0.3 | Filter envelope decay |
| `sweepMode` | choice | Up | Filter sweep direction (Off/Up/Down) |
| `freqTracking` | 0–1 | 0 | Filter frequency pitch tracking amount |
| `filterFreq` | −2000–4000 Hz | 0 | Manual filter frequency offset |
| `unisonVoices` | 1–8 (int) | 1 | Number of unison voices |
| `unisonDetune` | 0–100 ct | 20 | Unison detune spread in cents |
| `subLevel` | 0–1 | 0 | Sub oscillator level (one octave down) |
| `unisonBlend` | 0–1 | 0.5 | Unison voice blend (center vs. spread) |

### Notable Implementation Details

- Plugin is an **audio effect** (not a synth): `IS_SYNTH FALSE`, no MIDI
- `ENABLE_DEBUG_LOG` is defined only in the `build` (RelWithDebInfo) preset
- Logo SVG is embedded as binary data under the `JQGunkData` namespace via `juce_add_binary_data`
- APVTS state ID: `"Parameters"` — root XML tag is `<Parameters>`
- `waveform` choice indices 0/1/2 map to `WaveformType` Sine/Triangle/Square/Sawtooth (add 1 to get `WaveformType` enum value)
- Factory presets ship in `Presets/Factory/` and are installed by the `install-presets` CMake target
