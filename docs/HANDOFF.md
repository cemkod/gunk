# Handoff: plugin-review follow-up (branch `ADSR`)

Started on 2026-11-14 and updated on 2026-11-14, after the fifth session.

## Goal

Work through the recommendations in `plugin-review.md` at the repo root, in the order given there. The bug table is done. Review item 2 (drive and stereo unison) is built. Items 3–4 are next.

## State

All of this is committed on `ADSR`, one commit per fix, and none of it is pushed. Every change builds cleanly with `cmake --build build`. **None of it has been run in a DAW or the Standalone app, and none of it has been listened to.**

| Commit | Fix | Where |
| --- | --- | --- |
| `7eb2e66` | The IR survives a state restore and a re-prepare. The ADSR also triggers on the gate-open edge. This commit also contains the earlier ADSR work and six presets, which nobody has auditioned. | `Source/IRConvolutionEngine.h`, `Source/PluginProcessor.cpp:615` |
| `b12665f` | ADSR velocity is smoothed with a ~5 ms one-pole. A retrigger inside the 100 ms window keeps the running peak. | `Source/ADSREnvelope.h:62` |
| `8a177eb` | IR loads are issued from the audio thread. | `IRConvolutionEngine.h:39` (`loadIR` posts the file), `:52` (`applyPendingLoad`), `PluginProcessor.cpp:737` |
| `96b73b8` | `setWaveform` no longer allocates or frees on the audio thread. | `Source/Oscillator.cpp:23` (`basicTable`), `:52`, `:203` and `:285` (custom loaders) |
| `226afba` | The `transientDecay` parse limit is now 0.2 and the `transientDry` limit is 1.0. | `createParameterLayout` in `PluginProcessor.cpp` |
| `ae094a8` | `getTailLengthSeconds` returns a real value. | `PluginProcessor.cpp:377`, `IRConvolutionEngine.h:86` |
| `1411f61` | Stereo unison: per-osc WIDTH, MONO < crossover, stereo drive/filter/IR. Review item 2, second half. | `Oscillator.cpp` `processBlock`, `Source/SideHighpass.h`, `processBlock` from `// --- AUDIO GENERATORS ---` to the output loop |
| `fb2a919` | Dry path selectable: Clean or Through FX. | `dryRouting`, `processBlock` before `// --- DRIVE` |
| `8e80608` | Drive stage before the filter: Soft/Hard/Tube, 2x oversampling, auto-gain, Drive mod target. Review item 2, first half. | `Source/DriveEngine.h`, `Source/DriveSectionComponent.h`, `processBlock` before `// --- FILTER ---` |
| `38f609c` | Parameters missing from a loaded preset or session reset to their defaults instead of keeping the previous value. | `PresetManager::replaceStateFilling` |
| `209baf0` | Presets store IR and transient paths relative to their library folder, as wavetables already did. | `toPresetPath`/`fromPresetPath`/`restoreSamplePaths` in `PluginProcessor.cpp`, file choosers in `IRSectionComponent.h` and `TransientSectionComponent.h` |

The review's bug table is done. The review's open question about `<PARAM id="morph"/>` is answered: APVTS resets an empty `<PARAM>` to its default, but leaves a parameter with no `<PARAM>` child at its previous value (`juce_AudioProcessorValueTreeState.cpp:397-430` at 7.0.12). `38f609c` fixes the second case. **The fix changes how the old 13-parameter factory presets sound**: every parameter they don't mention is now at its default instead of carried over from the previous preset.

`pitch-debugger/` (433 MB) is deliberately untracked and not ignored, because the review names it as a possible starting point for tracking tests.

## Decisions

- **One `juce::dsp::Convolution` for the engine's lifetime, with no stored path to replay.** The JUCE 7.0.12 `prepare()` docs say it fully initialises the IR from the most recent `loadImpulseResponse()`. The old bug came from `make_unique` in `prepare()`.
- **IR thread-safety: `SpinLock` handoff, applied on the audio thread.** The JUCE `Convolution` class docs say: "load() calls must be synchronised with process() calls, which in practice means making the load() call from the audio thread". `juce_Convolution.cpp` confirms the race. `loadImpulseResponse()` writes `pendingCommand`, a plain non-atomic member, and `process()` retries `postPendingCommand()` on that same member.
  - The audio thread only try-locks. `prepare()` also calls `applyPendingLoad()`, because hosts never run it concurrently with `process()`. This keeps the state-restore fix working: the IR is loaded before `prepare()` runs.
  - `applyPendingLoad()` is called every block, before the `irEnabled` check. A load made while the section is bypassed would otherwise be lost.
  - Copying a `juce::File` inside the lock only increments the path string's refcount. `pendingFile` keeps its own reference, so nothing is freed on the audio thread.
- **Velocity: smooth, not latch.** The user picked this. The rejected options were latching the envelope-follower value at note-on (lags the pick, loses dynamics) and latching after a ~10 ms window (still steps once).
- **`setWaveform`: pointer swap onto shared static tables.** `frames` is now a `const Frames*` that points at one of the four function-local static tables or at the object's own `customFrames`.
  - `customFrames` is kept after switching to a basic shape. This costs memory but nothing reads it again, because a reload replaces it.
  - The class is now `JUCE_DECLARE_NON_COPYABLE`, because a copy would leave `frames` dangling.
- **Tail length formula.** The tail is the longer of the amp-envelope tail and the transient tail, plus the IR tail. Both envelopes feed the IR, and each tail counts its envelope's decay to -60 dB, which is ln(1000) ≈ 6.91 time constants.
  - **Envelope follower (amp source 0):** Q's coefficient is `exp(-2/(sps·dur))` (`build/_deps/cycfi_q-src/q_lib/include/q/fx/envelope.hpp:331`). So τ = 100 ms / 2 and the tail is about 0.35 s. The 100 ms comes from `PluginProcessor.cpp:339`; if that changes, update `getTailLengthSeconds`.
  - **ADSR (amp source 1):** τ = `modEnvRelease`.
  - **Transient:** attack + hold + decay.
  - **IR:** `getCurrentIRSize() / sampleRate`, published from the audio thread through `std::atomic<double> tailSeconds`, because `getCurrentIRSize()` reads the engine that `process()` swaps.
- **Preset paths are library-relative.** The user picked this over paths relative to the preset file. A file inside `~/.config/JQGunk/Impulse Responses/` or the new `~/.config/JQGunk/Transient Samples/` is saved relative to that folder. Anything else stays absolute. Session state (`getStateInformation`) still writes absolute paths, and every restore goes through `fromPresetPath`, which accepts either form.
  - Relative paths are written with `/`, and `toPresetPath` uses `isAChildOf`, which fixes the old `..`-prefix check for a path on another Windows drive.
  - No factory transient samples ship, so there is no install target for `Transient Samples/`. If the folder doesn't exist, the file chooser falls back to the home directory.
- **Drive scope:** the user picked "drive now, stereo later" and the mid-sized design: amount, curve and on/off, plus a mod target, with no Tone or Mix controls.
  - **Placement:** on `scratch.osc1Out` after the pre-filter mix. That includes the sub unless `subBypassFilter` is on, in which case the sub skips both drive and filter.
  - **Oversampling:** 2x `filterHalfBandPolyphaseIIR`, `isMaxQuality = false`. Its latency (a few samples) applies to the synth path only and isn't reported to the host, because the dry signal isn't delayed to match. When drive goes from off to on, the oversampler is reset and the gain jumps to its target.
  - **Auto-gain:** the output is scaled by `2·kRef / (f(g·kRef) − f(−g·kRef))` with kRef = 0.5. A 0.5-peak sine then stays within 3 dB of dry for every curve from 0 to 36 dB. Quiet inputs come out up to ~17 dB louder at full drive, which is the saturation compressing them, as intended. The osc mix is normally near full scale anyway, because the amp envelope is applied after the filter. These numbers come from a Python model of the curves, not from measuring the plugin.
  - **UI:** a 100 px Drive section in the osc row, between the combined-osc section and the transient section. The transient section had the most spare width. The editor size is unchanged.
- **Dry path (`dryRouting`): Clean / Through FX.** The user asked for the dry signal's routing to be selectable. In Clean (the default) the maths is unchanged: `(osc + sub)·env + dry`. In Through FX it becomes `filter(drive(osc·env + dry)) + sub·env`, where the sub term is present only when the sub bypasses the filter. The amp envelope moves ahead of drive in this mode, because the synth can't be separated from the dry signal after the nonlinearity. The dry signal is not enveloped. The control is a DRY PATH combo in the Output section.
- **Stereo scope:** the user picked a per-osc WIDTH with mod targets (`Width1`/`Width2` = 20/21) and a global MONO < crossover (default 150 Hz).
  - **Mid/side design:** `WavetableOscillator::processBlock` returns the old mono sum plus an optional side buffer. Each voice's pan is its detune position in [−1, 1] × width, with a linear law, so (L+R)/2 is exactly the old signal. With width at 0, or with one voice, the side is 0 and the output is L = R = the old mono output. Width ramps across each block, so modulation doesn't step.
  - The summed side is high-passed (12 dB/oct) before L/R are formed. That makes the low end identical in both channels, so it stays mono through the drive and filter too.
  - **Mono output bus:** no side buffer is passed, and the path is the old mono one, so there's no extra CPU. A stereo output runs drive, the filter (`envelopeFilterR`) and the IR twice, even at width 0. This was kept simple rather than optimised.
  - **IR:** now `Stereo::yes` with a two-channel spec. Mono IR files are used on both channels (confirmed in `juce_Convolution.cpp`), and all factory IRs are mono. A user's stereo IR file now plays true stereo; before, only its first channel was used.
  - **UI:** the osc slider rows shrank from 20/3 px to 18/2 px so a WIDTH row fits. MONO < sits next to VOL in Output.
- **Gate-open and onset are OR-ed** to trigger the ADSR. Gate-open catches the notes aubio misses, and the onset re-latches at the real pick peak.

## Next steps

1. **Test in a DAW or the Standalone app.** The user does this; an agent can't listen.
   - Load an IR, save the session, reopen it, and confirm the IR is still there.
   - Change the sample rate or buffer size with an IR loaded.
   - Load an IR while the IR section is off, then turn it on.
   - Play legato notes and hammer-ons in Env Module mode, and listen for bumps at note start and for whether 5 ms of velocity smoothing feels right.
   - Switch waveforms while playing. Load a multi-frame `.wt` file, then switch to a basic shape.
   - **Stereo:** set VOICES ≥ 2 and turn WIDTH up on each osc. Check that the spread sounds right and that MONO < keeps the lows centred. Check mono compatibility (sum to mono in the DAW) and that WIDTH 0 sounds exactly like before. Check the denser osc slider rows. Check that a mono track or mono output still works.
   - **Dry path:** switch DRY PATH to FX with DRY up, and check that the bass is driven and filtered. Then check that Clean sounds exactly as before. In FX mode with the ADSR as the amp source, the synth is enveloped before the drive, so the distortion fades with the release.
   - **Drive:** check the three curves at low and high drive, the level jump when toggling, zipper noise when modulating Drive from the LFO, and aliasing on high notes (Hard aliases most). Check that the section layout looks right.
   - **Old factory presets** (13 parameters): confirm that they now load the same way regardless of which preset came before. They will sound different from before, because unmentioned parameters are now at their defaults.
   - Save a user preset that uses an IR from `Impulse Responses/` and check that the `.jqgpreset` has a relative `irSamplePath`. Load an old preset that has absolute paths and check that it still works.
2. **Review items 3–4:** onset-reset of the pitch window, and the preset rebuild. The rebuild should set Drive explicitly now that it exists.

## Traps

- **Expect two triggers per picked note.** The gate usually opens a few ms before the aubio onset fires. `b12665f` stopped the second trigger from resetting velocity. Before that fix, it could collapse velocity to a near-zero sample in the middle of the attack, which is the most likely cause of any old note-start bump. If a bump remains, check `update()` restarting the attack.
- **`setWaveform` uses a try-lock and can silently do nothing.** This is deliberate. `updateOscParams` (`PluginProcessor.cpp:411`) compares the oscillator with the parameter every block and retries. The `setStateInformation` calls rely on the same retry, because a restore sets `paramWhenCustomLoaded = -1`. Don't switch it back to a blocking lock.
- **Don't call `convolution.getCurrentIRSize()` or `getLatency()` off the audio thread.** Use `IRConvolutionEngine::getTailSeconds()`.
- **A long ADSR release gives a long reported tail.** At the 4 s maximum, the tail is about 28 s. That is mathematically correct, but some hosts may keep processing for a long time after you stop. This hasn't been checked.
- **The `-Warray-bounds` warning in `PitchDetectorThread::drainFifo`** (`Source/PitchDetectorThread.h:122`, from `std::sort` on a `float[5]`) predates these sessions and was left alone. It may be a false alarm from GCC.
- **New `ModTarget` values must be appended.** Presets store the target as a choice index, and `ModMatrixComponent.cpp` builds the combo from `kTargetNames` with item ID = index + 1 = enum value. The enum, `kTargetNames` and `kTargetScale` have to stay aligned.
- **Attach a `ComboBoxAttachment` after adding the combo's items.** An attachment created first can't apply its initial selection. `DriveSectionComponent` and the IR section create theirs afterwards. `OutputSectionComponent` attaches first, which may leave its AMP ENV combo blank until the value changes. That hasn't been checked.
- **The clangd errors in `PluginProcessor.cpp` are not real.** They include a missing `std::span`, no `_dB` literal and "no member `applyPendingLoad`". The CMake build compiles cleanly, so trust it over the editor diagnostics.
- **Don't search `/` for JUCE headers.** It timed out after 120 s. JUCE isn't in `build/_deps`, so fetch the files from GitHub at tag `7.0.12` (`modules/juce_dsp/frequency/juce_Convolution.{h,cpp}`). Q *is* in `build/_deps/cycfi_q-src`, docs included.
- **Use `pipestatus`, not `PIPESTATUS`, in this shell.** The user's shell is zsh, so `${PIPESTATUS[0]}` comes back empty. Check build success from the output instead.
