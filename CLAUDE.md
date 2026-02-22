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
```

Build artifacts land in `build/JQGunk_artefacts/` (VST3 + Standalone).

There are no automated tests. Verification is done by loading the plugin in a DAW or running the Standalone app.

## Architecture

Signal flow: **Audio Input → Pitch Detector → Wavetable Oscillator → Envelope Gate → Resonant Filter → Mix → Output**

### Key Files

- **`Source/PluginProcessor.h/.cpp`** — Core audio engine. Contains:
  - `AutocorrelationPitchDetector`: FFT-based McLeod Pitch Method, 40–400 Hz, 8192-sample buffer with 128-sample hop, hysteresis gating
  - `ResonantLowpassFilter`: Simple SVF for envelope-controlled auto-wah
  - `JQGunkAudioProcessor`: Main JUCE `AudioProcessor`, owns APVTS, orchestrates all DSP
- **`Source/Oscillator.h/.cpp`** — `WavetableOscillator`: 2048-point wavetable, 4 built-in waveforms (Sine/Triangle/Square/Sawtooth), custom WAV loading, SpinLock for thread-safe wavetable swapping
- **`Source/PluginEditor.h/.cpp`** — `JQGunkAudioProcessorEditor`: custom JUCE editor, 5 rotary sliders, 5 waveform buttons, 3-way Sweep radio group, live frequency label (timer-driven)
- **`Source/LookAndFeel.h`** — `BassLookAndFeel` (extends `LookAndFeel_V4`): dark navy/purple theme with yellow accent (`#fffff100`)

### APVTS Parameters

| ID | Range | Default | Description |
|----|-------|---------|-------------|
| `level` | 0–1 | 0.8 | Output level |
| `mix` | 0–1 | 1.0 | Dry/wet blend |
| `waveform` | choice | Sine | Oscillator waveform |
| `envSensitivity` | 0–7 | 3.0 | Envelope follower sensitivity |
| `envResonance` | 0–8 | 2.0 | Filter Q |
| `envDecay` | 0.01–2 | 0.3 | Filter envelope decay (seconds) |
| `sweepMode` | choice | Up | Filter sweep direction (Off/Up/Down) |

### Notable Implementation Details

- Plugin is an **audio effect** (not a synth): `IS_SYNTH FALSE`, no MIDI
- `ENABLE_DEBUG_LOG` is defined only in the `build` (RelWithDebInfo) preset
- Logo SVG is embedded as binary data under the `JQGunkData` namespace via `juce_add_binary_data`

