# Mod Matrix

JQ Gunk includes an 8-slot modulation matrix that routes signal sources to synthesis parameters. Each slot independently adds a scaled offset to its target parameter once per audio block.

---

## Data Model (`Source/ModMatrix.h`)

### Sources (`ModSource` enum)

| Index | Name | Value range | Description |
|-------|------|-------------|-------------|
| 0 | None | 0 | No modulation |
| 1 | Envelope | 0–1 | Amplitude follower (gate envelope) |
| 2 | Pitch | 0–1 | Detected pitch: `(pitchHz - 40) / 360`, clamped 0–1 (maps 40–400 Hz) |
| 3 | Mod Env | 0–1 | Slew-limited modulation envelope (user-controlled attack/decay) |
| 4 | LFO | varies | LFO output, scaled by LFO amount |

### Targets (`ModTarget` enum)

`ModTarget` integer values start at 1 (not 0). This is significant — see the APVTS offset quirk below.

| Enum value | Name | Scale factor | Target parameter units |
|------------|------|--------------|------------------------|
| 1 | None | 0 | — |
| 2 | Morph1 | 1 | 0–1 |
| 3 | Morph2 | 1 | 0–1 |
| 4 | FilterFreq | 6000 | Hz offset (-2000–4000) |
| 5 | FilterRes | 8 | Q (0–8) |
| 6 | Osc1Level | 2 | Level (0–2) |
| 7 | Osc2Level | 2 | Level (0–2) |
| 8 | Unison1Detune | 100 | Cents (0–100) |
| 9 | SubLevel | 1 | Level (0–1) |
| 10 | Glide | 1 | Seconds (0–1) |
| 11 | Unison2Detune | 100 | Cents (0–100) |
| 12 | LfoRate | 20 | Hz (0.01–20) |
| 13 | MasterVolume | 2 | Level (0–2) |
| 14 | Osc1FineTune | 200 | Cents (±200 ct = ±2 semitones) |
| 15 | Osc2FineTune | 200 | Cents (±200 ct = ±2 semitones) |
| 16 | LfoAmount | 1 | Amount (0–1) |
| 17 | Unison1Blend | 1 | Blend (0–1) |
| 18 | Unison2Blend | 1 | Blend (0–1) |

### `Slot` struct

```cpp
struct Slot { int source = 0; int target = 0; float amount = 0.0f; };
std::array<Slot, 8> slots {};
```

Eight slots; `source` and `target` are integer indices matching the enum values above.

### Modulation formula

```
offset = amount × sourceVal × kTargetScale[target]
```

- `amount` range: -3.0 to 3.0 (APVTS parameter)
- `sourceVal` range: 0–1 (normalized source signal)
- `kTargetScale[target]`: scales offset to the target's parameter units

`getOffset(ModTarget)` sums this formula across all 8 slots that share the same target.

---

## Per-block flow

### `snapshot()` — called once at the top of `processBlock()`

```cpp
modMatrix.snapshot(apvts, envelope, modEnvelope, glide.getLastDetectedFreq(), lfoScaled);
```

1. Captures `sourceVals[0..4]` from the arguments (None=0, envelope, pitch normalized, modEnvelope, lfo).
2. Reads all 8 slot assignments from APVTS atomics.

**APVTS offset quirk:** `ModTarget` starts at 1 but APVTS choices are 0-indexed. `snapshot()` adds +1 when reading the target choice:

```cpp
slots[i].target = (int) apvts.getRawParameterValue("modSlot" + n + "Target")->load() + 1;
```

So APVTS choice index 0 ("None") maps to `ModTarget::None` (= 1), choice index 1 ("Morph 1") maps to `ModTarget::Morph1` (= 2), etc.

---

## Integration in `PluginProcessor` (`Source/PluginProcessor.cpp`)

### Applying offsets — `processBlock()` lines 468–499

After `snapshot()`, offsets are applied to block-local parameters before DSP:

```cpp
// Scalar params — clamped immediately
p.oscLevel   = jlimit(0, 2,       p.oscLevel   + modMatrix.getOffset(ModTarget::Osc1Level));
p.osc2Level  = jlimit(0, 2,       p.osc2Level  + modMatrix.getOffset(ModTarget::Osc2Level));
p.subLevel   = jlimit(0, 1,       p.subLevel   + modMatrix.getOffset(ModTarget::SubLevel));
p.filterFreq = jlimit(-2000,4000, p.filterFreq + modMatrix.getOffset(ModTarget::FilterFreq));
p.resonance  = jlimit(0, 8,       p.resonance  + modMatrix.getOffset(ModTarget::FilterRes));
p.glideTime  = jlimit(0, 1,       p.glideTime  + modMatrix.getOffset(ModTarget::Glide));
masterVolume = jlimit(0, 2,       masterVolume + modMatrix.getOffset(ModTarget::MasterVolume));

// Fine tune — offset in cents converted to pitch multiplier
p.osc1PitchMult *= pow(2, modMatrix.getOffset(ModTarget::Osc1FineTune) / 1200.0f);
p.osc2PitchMult *= pow(2, modMatrix.getOffset(ModTarget::Osc2FineTune) / 1200.0f);

// LFO rate/amount — LFO object owns state across blocks
lfo.updateModulated(baseRate, modMatrix.getOffset(ModTarget::LfoRate),
                    baseAmount, modMatrix.getOffset(ModTarget::LfoAmount));

// Per-oscillator params — handled in updateOscParams()
updateOscParams({ oscillator, ..., ModTarget::Unison1Detune, ModTarget::Unison1Blend, ModTarget::Morph1, ... });
updateOscParams({ osc2,       ..., ModTarget::Unison2Detune, ModTarget::Unison2Blend, ModTarget::Morph2, ... });
```

### `updateOscParams()` — lines 358–390

Reads Unison Detune, Blend, and Morph offsets from the mod matrix and applies them to the oscillator. Stores the final modulated values in atomics for UI readback:

```cpp
cfg.lastDetuneOffset.store(detuneOffset);  // → lastModDetuneOffset / lastModDetune2Offset
cfg.lastBlendOffset .store(blendOffset);   // → lastModBlendOffset  / lastModBlend2Offset
cfg.lastMorphModulated.store(modulated);   // → lastModulatedMorph  / lastModulatedMorph2
```

### `OscUpdateConfig` struct — lines 140–155

Bundles all per-oscillator parameter IDs and mod targets so `updateOscParams()` is shared between OSC 1 and OSC 2:

```cpp
struct OscUpdateConfig {
    WavetableOscillator& osc;
    int&                 paramWhenCustomLoaded;
    const char*          waveformId, *voicesId, *detuneId, *blendId, *morphId;
    ModTarget            detuneTarget, blendTarget, morphTarget;
    std::atomic<float>&  lastDetuneOffset, lastBlendOffset, lastMorphModulated;
};
```

### Atomics for UI readback

Declared in `PluginProcessor.h` lines 177–182:

| Atomic | Written by | Used by |
|--------|-----------|---------|
| `lastModulatedMorph` | `updateOscParams` (OSC 1) | UI morph display |
| `lastModulatedMorph2` | `updateOscParams` (OSC 2) | UI morph display |
| `lastModDetuneOffset` | `updateOscParams` (OSC 1) | UI detune display |
| `lastModDetune2Offset` | `updateOscParams` (OSC 2) | UI detune display |
| `lastModBlendOffset` | `updateOscParams` (OSC 1) | UI blend display |
| `lastModBlend2Offset` | `updateOscParams` (OSC 2) | UI blend display |

---

## APVTS Parameters

Declared in `PluginProcessor.cpp` lines 256–273. For each slot `n` (0–7):

| Parameter ID | Type | Range | Default | Description |
|---|---|---|---|---|
| `modSlot[n]Source` | AudioParameterChoice | 5 options | 0 (None) | Source signal |
| `modSlot[n]Target` | AudioParameterChoice | 18 options | 0 (None) | Target parameter |
| `modSlot[n]Amount` | AudioParameterFloat | -3.0 to 3.0 | 0.0 | Modulation depth |

---

## UI (`Source/ModMatrixComponent.h/.cpp`)

The `ModMatrixComponent` renders 8 `SlotRow` structs, each with source combo, target combo, and amount slider — all APVTS-attached.

**Layout:**
- Total height: 24px header + 8 × 34px rows = **296px**
- Column widths: 24px slot# | 150px source | 6px gap | 170px target | 6px gap | remainder (amount)

**Target combo grouped menus** (`setupTargetComboMenu`):

| Submenu | Targets |
|---------|---------|
| OSC 1 | Level, Morph, Fine Tune, Unison Detune, Unison Blend |
| OSC 2 | Level, Morph, Fine Tune, Unison Detune, Unison Blend |
| Filter | Frequency, Resonance |
| LFO | Rate, Amount |
| Global | Glide, Sub Level, Master Volume |

Odd rows receive a subtle background fill via `paint()` (alternating row shading).

---

## Key Design Decisions

1. **Two envelope sources.** The gate `Envelope` tracks amplitude fast (attack ~10ms, release ~100ms) and is primarily for auto-wah. `Mod Envelope` is slew-limited with user-adjustable attack/decay, better suited for slower sweeps and rhythmic modulation.

2. **Per-oscillator routing.** Morph, Detune, and Blend each have separate OSC 1 / OSC 2 targets, so the two oscillators can be modulated independently.

3. **Atomic readback.** Modulated oscillator values (morph, detune, blend) are stored in `std::atomic<float>` so the UI timer thread can read them lock-free without touching the audio thread's working copies.

4. **Clamping after modulation.** Every modulated parameter is clamped to its valid range immediately after the offset is added, preventing out-of-range values from reaching the DSP.

5. **LFO rate/amount persistence.** Unlike other targets where the offset is added to a block-local variable, LFO modulation calls `lfo.updateModulated()` so the LFO object stores the effective rate and amount and applies them on its next `tick()`.
