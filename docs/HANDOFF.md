# Handoff: plugin-review follow-up (branch `ADSR`)

Written on 2026-11-14.

## Goal

Work through the recommendations in `plugin-review.md` at the repo root, in the order given there. Item 1 covers two fixes: the impulse response being lost on session reload, and the ADSR trigger.

## State

Committed on `ADSR`, not pushed:

- **IR survives state restore and re-prepare** (`Source/IRConvolutionEngine.h`). This fixes the first two rows of the review's bug table. It builds and has not been tested in a DAW.
- **ADSR also triggers on the gate-open edge** (`Source/PluginProcessor.cpp:589`). Velocity tracking uses the same `noteOn`. It builds and has not been listened to.
- The same commit also contains ADSR work that was already in progress before this session: `ADSREnvelope.h` and the edits to the envelope, gate, onset and editor files. It also adds the six new presets, which are still not auditioned, and the review and docs files.

Verification so far is limited to `cmake --build build` succeeding. Nothing has been run.

Still open from the review's bug table:

- ADSR velocity steps during the attack.
- `setWaveform` runs on the audio thread.
- Two text-entry ranges are wrong.
- Absolute paths in presets.
- `getTailLengthSeconds` returns 0.

## Decisions

- **One `juce::dsp::Convolution` for the engine's lifetime, with no stored path to replay.** The review suggested storing the IR path and reloading it in `prepare()`. That was not done, because the JUCE 7.0.12 `Convolution::prepare()` docs say: "Calling prepare() will ensure that the IR supplied to the most recent call to loadImpulseResponse() is fully initialised", and they recommend loading *before* `prepare()`. The old bug came from `prepare()` creating a new object with `make_unique` each time. A `prepared` flag replaces the old null check in `processBlock`.
  - JUCE is not on disk: `build/_deps` has no JUCE checkout. The docs were read from the raw GitHub header at tag 7.0.12.
- **Gate-open and onset are OR-ed, not one or the other.** Gate-open catches the notes aubio misses. The onset still re-triggers on picked notes, which re-latches velocity at the real pick peak.
- **What was committed.** The user chose "code + docs + ignore rest". `.gitignore` now covers the AUR build output, `perf.data*` and `cpu-log.csv`. `pitch-debugger/` (433 MB) was deliberately left untracked and *not* ignored, because the review names it as a possible starting point for tracking tests.

## Next steps

1. Test both fixes in a DAW or the Standalone app:
   - Load an IR, save the session, reopen it, and confirm the IR is still there.
   - Change the sample rate or buffer size with an IR loaded.
   - Play legato notes and hammer-ons in Env Module mode.
2. Fix ADSR velocity stepping during the attack, in `ADSREnvelope::trackVelocity` (`Source/ADSREnvelope.h`). Velocity is a raw peak that keeps rising for 100 ms and multiplies the envelope directly (`PluginProcessor.cpp:593`). Smooth it, or latch it once. Pick one with the user.
3. Make IR loading thread-safe. `loadIRFromFile` (`PluginProcessor.cpp:838`) calls `loadImpulseResponse` from the message thread while `process()` runs. The JUCE docs say "load() calls must be synchronised with process() calls, which in practice means making the load() call from the audio thread". This is not in the review. The behaviour predates this session and has not been seen to cause a problem.
4. Continue down the review's bug table: `setWaveform` off the audio thread, the parse ranges in `createParameterLayout`, and `getTailLengthSeconds`. After that, review items 2–4 cover drive and stereo unison, onset-reset of the pitch window, and the preset rebuild.

## Traps

- **Expect two triggers per picked note.** The gate usually opens a few milliseconds before the aubio onset fires, so the ADSR gets two note-ons. `update()` restarts the attack from the current value, not from zero, so this should be inaudible, but nobody has listened yet. If there is a bump at note start, look here before blaming the envelope maths.
- **The clangd errors in `PluginProcessor.cpp` are not real build errors.** They report a missing `std::span`, no `_dB` literal and a broken APVTS constructor. The CMake build compiles cleanly; the editor's clangd is probably not using the project's C++20 settings.
- **Don't search `/` for JUCE headers.** It timed out after 120 s. Fetch the header from GitHub at tag `7.0.12` instead.
- **The old `loadIR` comment calling it "Thread-safe" was wrong** and has been removed. Don't restore it; see next step 3.
