# JQ Gunk

A JUCE-based audio effect plugin (VST3 + Standalone) for bass synthesis and processing.

JQ Gunk detects the pitch of an incoming bass signal and synthesises oscillator tones at that frequency, shaped by an envelope-controlled resonant filter (auto-wah). The result can be blended with the dry signal.

## Signal Flow

**Audio In → Pitch Detector → Oscillators (OSC 1 + OSC 2 + Sub) → Envelope Gate → Resonant Filter → Mix → Audio Out**

## Features

- **Pitch tracking** — autocorrelation-based McLeod Pitch Method, 40–400 Hz
- **Two wavetable oscillators** — Sine, Triangle, Square, Sawtooth, or custom WAV/.wt file; independent tuning (octave, coarse, fine), unison, and morph per oscillator
- **Unison** — up to 8 voices per oscillator with detune spread and blend control
- **Sub oscillator** — configurable octave shift (-2 to 0), optional filter bypass
- **Pitch glide** — configurable portamento time
- **Resonant filter** — SVF-based with LP / HP / BP modes, envelope-controlled auto-wah sweep
- **Frequency tracking** — filter cutoff follows detected pitch (0–100%)
- **LFO** — Sine, Triangle, Square, Sawtooth shapes; 0.01–20 Hz; routable via mod matrix
- **Modulation matrix** — 8 slots, 5 sources (Envelope, Pitch, Mod Envelope, LFO, None) × 18 targets
- **Modulation envelope** — dedicated attack/decay envelope for mod matrix sources
- **Transient player** — load a WAV/AIFF sample triggered on each gate open, with pitch tracking and offset
- **Gate** — threshold + hysteresis to suppress noise when no bass is playing
- **Dry level** — blend the processed signal with the original
- **192 factory wavetables** — from the Surge XT library (Basic, Generated, Oneshot, Rhythmic, Sampled, Waldorf categories)

## Platform Support

Pre-built binaries are produced by CI for:

| Platform | Format |
|----------|--------|
| Linux | VST3 + Standalone (DEB + RPM packages) |
| Windows | VST3 (ZIP) |
| macOS | VST3 + Standalone (DMG) |

## Building

Requires CMake 3.22+, Ninja, and a C++17 compiler.

```bash
# Debug build (RelWithDebInfo + ENABLE_DEBUG_LOG)
cmake --preset build
cmake --build build

# Release build (with LTO)
cmake --preset release
cmake --build build-release

# Windows cross-compile from Linux (requires mingw-w64)
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

## Installing Factory Wavetables

```bash
cmake --build build --target install-wavetables
```

Copies the factory wavetable library to `~/.config/JQGunk/Wavetables/`. 192 files across 6 categories (Basic, Generated, Oneshot, Rhythmic, Sampled, Waldorf) in `.wt` and `.wav` formats.

### Third-party content

The wavetables under `Wavetables/Surge/` are taken from the [Surge XT](https://github.com/surge-synthesizer/surge) synthesizer project and redistributed under the GNU General Public License v3, the same license as this project. See `Wavetables/Surge/NOTICE` for full attribution.

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

### Gate

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Gate Threshold | 0.001–0.04 | 0.003 | Input level required to open the gate |
| Gate Hysteresis | 0–6 dB | 3.5 | Extra headroom to keep the gate open |
| Glide | 0–1 s | 0 | Pitch portamento time |
| Dry Level | 0–1 | 0 | Amount of original dry signal in the output |

### Oscillator 1 & Oscillator 2 (independent)

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Waveform | Triangle / Square / Sawtooth | Triangle | Built-in waveform or custom wavetable |
| Level | 0–2 | 1.0 | Oscillator output level |
| Morph | 0–1 | 0 | Wavetable frame morph position |
| Octave Shift | -2 / -1 / 0 / +1 / +2 | 0 | Octave transposition |
| Coarse Tune | -24–+24 st | 0 | Semitone offset |
| Fine Tune | -100–+100 ct | 0 | Cent offset |
| Unison Voices | 1–8 | 1 | Number of unison voices |
| Unison Detune | 0–100 ct | 20 | Detune spread in cents |
| Unison Blend | 0–1 | 0.5 | Center vs. spread blend |

### Sub Oscillator

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Sub Level | 0–1 | 0 | Sub oscillator output level |
| Sub Octave | -2 / -1 / 0 / +1 | -1 | Octave offset relative to detected pitch |
| Sub Bypass Filter | On / Off | On | Skip the resonant filter for the sub signal |

### Filter

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Filter Type | LP / HP / BP | LP | SVF filter mode |
| Resonance | 0–8 | 2.0 | Filter Q |
| Filter Freq | -2000–+4000 Hz | 550 | Manual filter frequency offset |
| Freq Tracking | 0–1 | 1.0 | How much the filter tracks detected pitch |

### LFO

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| LFO Rate | 0.01–20 Hz | 1.0 | LFO frequency |
| LFO Shape | Sine / Tri / Square / Saw | Sine | LFO waveform |
| LFO Amount | 0–1 | 1.0 | LFO depth (scales mod matrix amounts) |

### Modulation Envelope

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Mod Env Attack | 0.001–2 s | 0.01 | Mod envelope attack time |
| Mod Env Decay | 0.001–2 s | 0.1 | Mod envelope decay time |

### Transient Player

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Transient Level | 0–1 | 0.5 | Transient sample output level |
| Transient Slope | 0–5 | 3.1 | Gate sensitivity for transient triggering |
| Transient Pitch | -48–+48 st | 0 | Pitch offset for frequency-tracked playback |
| Transient Attack | 0.0001–0.01 s | 0.005 | Sample envelope attack |
| Transient Decay | 0.0001–0.05 s | 0.015 | Sample envelope decay |

### Output

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Master Volume | 0–2 | 1.0 | Output gain |

## Modulation Matrix

8 independent slots, each with a **Source**, **Target**, and **Amount** (-3.0 to +3.0).

**Sources:** None, Envelope, Pitch, Mod Envelope, LFO

**Targets include:** Oscillator morph, filter frequency, oscillator levels, sub level, unison detune, LFO rate, and more.

## Preset File Format

Presets are XML files with a `.jqgpreset` extension:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<Parameters>
  <PARAM id="gateThreshold" value="0.003162"/>
  <PARAM id="waveform" value="0"/>
  ...
</Parameters>
```

Factory presets ship in `Presets/Factory/` and are installed by the `install-presets` CMake target.
