# JQ Gunk

A JUCE-based audio effect plugin (VST3 + Standalone) for bass synthesis and processing.

JQ Gunk detects the pitch of an incoming bass signal and synthesises a clean oscillator tone at that frequency, shaped by an envelope-controlled resonant filter (auto-wah). The result can be blended with the dry signal.

## Signal Flow

**Audio In → Pitch Detector → Wavetable Oscillator (+ Unison + Sub) → Envelope Gate → Resonant Filter → Mix → Audio Out**

## Features

- **Pitch tracking** — autocorrelation-based McLeod Pitch Method, 40–400 Hz
- **Wavetable oscillator** — Sine, Triangle, Square, Sawtooth, or custom WAV
- **Unison** — up to 8 voices with detune spread and blend control
- **Sub oscillator** — one octave below at adjustable level
- **Pitch glide** — configurable portamento time
- **Envelope filter** — SVF-based resonant lowpass with auto-wah sweep (Up / Down / Off)
- **Frequency tracking** — filter cutoff follows detected pitch
- **Gate** — threshold + hysteresis to suppress noise when no bass is playing
- **Mix** — dry/wet blend

## Building

Requires CMake 3.22+, Ninja, and a system-installed JUCE.

```bash
# Debug build (RelWithDebInfo + ENABLE_DEBUG_LOG)
cmake --preset build
cmake --build build

# Release build (with LTO)
cmake --preset release
cmake --build build-release

# Windows cross-compile (requires mingw-w64: yay -S mingw-w64-toolchain)
# Run the Linux build first so moduleinfo.json is available.
cmake --build build
cmake --preset windows
cmake --build build-windows
```

Build artefacts land in `build/JQGunk_artefacts/`, `build-release/JQGunk_artefacts/`, and `build-windows/JQGunk_artefacts/` respectively.

## Installing Factory Presets

```bash
cmake --build build --target install-presets
```

Copies the 8 factory presets to `~/.config/JQGunk/Factory Presets/`. User presets are saved to `~/.config/JQGunk/User Presets/`.

### Factory Presets

| Preset | Description |
|--------|-------------|
| Default | Neutral starting point |
| Envelope Down | Downward filter sweep |
| Funky Wah | Classic auto-wah tone |
| Glide Machine | Smooth pitch portamento |
| Pitch Track | Filter follows pitch closely |
| Sub Rumble | Heavy sub-octave presence |
| Thick Saw | Sawtooth with unison spread |
| Wide Chorus | Wide unison detune blend |

## Parameters

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Gate Threshold | 0.001–0.04 | 0.01 | Input level required to open the gate |
| Gate Hysteresis | 0–6 dB | 3.5 | Extra headroom to keep the gate open |
| Glide | 0–1 s | 0 | Pitch portamento time |
| Mix | 0–1 | 1.0 | Dry/wet blend |
| Waveform | choice | Triangle | Triangle / Square / Sawtooth |
| Env Sensitivity | 0–7 | 3.0 | Envelope follower sensitivity |
| Env Resonance | 0–8 | 2.0 | Filter Q |
| Env Decay | 0.01–2 s | 0.3 | Filter envelope decay time |
| Sweep Mode | choice | Up | Filter sweep direction (Off / Up / Down) |
| Freq Tracking | 0–1 | 0 | How much the filter tracks detected pitch |
| Filter Freq | −2000–4000 Hz | 0 | Manual filter frequency offset |
| Unison Voices | 1–8 | 1 | Number of unison voices |
| Unison Detune | 0–100 ct | 20 | Detune spread in cents |
| Unison Blend | 0–1 | 0.5 | Center vs. spread blend |
| Sub Level | 0–1 | 0 | Sub oscillator level |

## Preset File Format

Presets are XML files with a `.jqgpreset` extension:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<Parameters>
  <PARAM id="gateThreshold" value="0.01"/>
  <PARAM id="waveform" value="1"/>
  ...
</Parameters>
```
