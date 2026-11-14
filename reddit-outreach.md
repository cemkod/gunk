# Reddit Outreach — JQ Gunk v0.2.1

## Target Subreddits

| Subreddit | Priority |
|---|---|
| r/AudioPlugins | High |
| r/WeAreTheMusicMakers | High |
| r/BassGuitar | High |
| r/Bass | Medium |
| r/synthesizers | Medium |
| r/edmproduction | Low |

---

## r/AudioPlugins / r/WeAreTheMusicMakers

**Title:** I made a free open-source bass synthesis plugin (VST3, Linux/Win/macOS) — early release, looking for feedback

Hey. I've been building a JUCE audio effect plugin called **JQ Gunk** and just released v0.2.1. It's early but functional enough to share.

**What it does:** you feed it a bass signal and it detects the pitch (McLeod Pitch Method, 40–400 Hz). Two wavetable oscillators + a sub oscillator generate tones at that frequency, which go through an SVF resonant filter (LP/HP/BP). The filter isn't envelope-controlled on its own — you wire it up yourself using the mod matrix.

**Features:**
- Real-time pitch tracking → oscillator synthesis
- Two independent wavetable oscillators (Triangle/Square/Sawtooth or custom WAV/.wt) with up to 8 unison voices, morph, and per-oscillator tuning
- 192 factory wavetables from the Surge XT library
- Sub oscillator with octave and filter-bypass controls
- SVF filter — LP/HP/BP
- LFO (Sine/Tri/Square/Saw, 0.01–20 Hz)
- 8-slot modulation matrix — sources include Envelope, Pitch, Mod Envelope, and LFO; wire up whatever you want (e.g. envelope → filter cutoff for auto-wah)
- Transient player — load a WAV/AIFF sample that fires on each gate open, pitch-tracked

Free and open source (GPL-3): https://github.com/cemkod/gunk

Pre-built binaries for Linux (DEB/RPM), Windows (ZIP), and macOS (DMG). The factory presets are from an earlier version so treat them as rough starting points — the interesting stuff comes from patching the mod matrix yourself. Feedback on sound, UX, or anything broken is very welcome.

---

## r/BassGuitar / r/Bass

**Title:** I built a free plugin that tracks your bass pitch and synthesises a synth layer over it — looking for players to try it

I'm a developer and bass player and I built a plugin for my own use that I'm now releasing: **JQ Gunk** v0.2.1.

It's an audio effect — your bass DI goes in. It detects your pitch in real time and generates oscillator tones at that frequency. Those go through a resonant filter (LP/HP/BP). You can blend it with your clean signal.

The filter doesn't auto-wah on its own — but you can route the envelope follower to the filter cutoff in the mod matrix and get exactly that. Same goes for the LFO, pitch, and a dedicated mod envelope. It's a modular setup, so you build the signal path you want.

It also ships with 192 wavetables from the Surge XT synthesizer, so there's a lot of raw material to work with beyond the basic waveforms.

Free + open source (GPL-3): https://github.com/cemkod/gunk — Linux/Windows/macOS.

The factory presets are from an earlier version so they may sound off — I'd recommend starting from scratch and experimenting. Would really value feedback from bass players on what sounds wrong, what feels clunky, or what's missing.

---

## r/synthesizers

**Title:** Free open-source pitch-tracking bass synthesis plugin — detects your bass input, synthesises oscillators at that pitch

Built a plugin that might interest people here: **JQ Gunk** v0.2.1 (free, GPL-3).

It's an audio effect that listens to a bass signal, detects the pitch (McLeod Pitch Method), and drives two wavetable oscillators at that frequency. You shape the result with an SVF filter (LP/HP/BP) and an 8-slot mod matrix where you can route the envelope follower, LFO, pitch signal, or a dedicated mod envelope to whatever targets you want — filter cutoff, morph, oscillator levels, unison detune, etc.

It ships with 192 wavetables from the Surge XT library (Basic, Generated, Oneshot, Rhythmic, Sampled, Waldorf categories), which gives a lot of ground to cover.

The presets are holdovers from an earlier version — better to ignore them and patch it from scratch. That's where the fun is anyway.

https://github.com/cemkod/gunk — Linux/Windows/macOS VST3. Early release, feedback welcome.
