# Handoff: plugin-review follow-up (branch `ADSR`)

Started on 2026-11-14 and updated on 2026-11-14, after the second session.

## Goal

Work through the recommendations in `plugin-review.md` at the repo root, in the order given there. The bug table is almost done. Review items 2–4 come after it.

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

Still open from the review's bug table: **absolute paths in presets** (transient sample and IR). The first session handoff listed "ADSR velocity steps during the attack" as open; it is now fixed (`b12665f`).

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
- **Gate-open and onset are OR-ed** to trigger the ADSR. Gate-open catches the notes aubio misses, and the onset re-latches at the real pick peak.

## Next steps

1. **Test in a DAW or the Standalone app.** The user does this; an agent can't listen.
   - Load an IR, save the session, reopen it, and confirm the IR is still there.
   - Change the sample rate or buffer size with an IR loaded.
   - Load an IR while the IR section is off, then turn it on.
   - Play legato notes and hammer-ons in Env Module mode, and listen for bumps at note start and for whether 5 ms of velocity smoothing feels right.
   - Switch waveforms while playing. Load a multi-frame `.wt` file, then switch to a basic shape.
2. **Absolute paths in presets.** This needs the user's decision before any code. The last proposal was to store paths relative to the `~/.config/JQGunk/...` folders, with an absolute-path fallback; the alternative is paths relative to the preset file. The paths are written in `getStateInformation` and read in `setStateInformation` and in the preset-load path. Both are in `PluginProcessor.cpp`: grep for `irSamplePath` and `transientSamplePath`.
3. **Review items 2–4** in `plugin-review.md`: drive/saturation and stereo unison, onset-reset of the pitch window, and the preset rebuild. These are feature work, so agree on scope with the user first.

## Traps

- **Expect two triggers per picked note.** The gate usually opens a few ms before the aubio onset fires. `b12665f` stopped the second trigger from resetting velocity. Before that fix, it could collapse velocity to a near-zero sample in the middle of the attack, which is the most likely cause of any old note-start bump. If a bump remains, check `update()` restarting the attack.
- **`setWaveform` uses a try-lock and can silently do nothing.** This is deliberate. `updateOscParams` (`PluginProcessor.cpp:411`) compares the oscillator with the parameter every block and retries. The `setStateInformation` calls rely on the same retry, because a restore sets `paramWhenCustomLoaded = -1`. Don't switch it back to a blocking lock.
- **Don't call `convolution.getCurrentIRSize()` or `getLatency()` off the audio thread.** Use `IRConvolutionEngine::getTailSeconds()`.
- **A long ADSR release gives a long reported tail.** At the 4 s maximum, the tail is about 28 s. That is mathematically correct, but some hosts may keep processing for a long time after you stop. This hasn't been checked.
- **The `-Warray-bounds` warning in `PitchDetectorThread::drainFifo`** (`Source/PitchDetectorThread.h:122`, from `std::sort` on a `float[5]`) predates these sessions and was left alone. It may be a false alarm from GCC.
- **The clangd errors in `PluginProcessor.cpp` are not real.** They include a missing `std::span`, no `_dB` literal and "no member `applyPendingLoad`". The CMake build compiles cleanly, so trust it over the editor diagnostics.
- **Don't search `/` for JUCE headers.** It timed out after 120 s. JUCE isn't in `build/_deps`, so fetch the files from GitHub at tag `7.0.12` (`modules/juce_dsp/frequency/juce_Convolution.{h,cpp}`). Q *is* in `build/_deps/cycfi_q-src`, docs included.
- **Use `pipestatus`, not `PIPESTATUS`, in this shell.** The user's shell is zsh, so `${PIPESTATUS[0]}` comes back empty. Check build success from the output instead.
