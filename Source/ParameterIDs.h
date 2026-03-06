#pragma once

// Compile-time parameter ID constants.
// Include this header wherever APVTS parameter IDs are referenced as string literals.
// All constants are constexpr const char* — zero runtime cost.

namespace ParamIDs {

    // Gate
    inline constexpr const char* gateThreshold  = "gateThreshold";
    inline constexpr const char* gateHysteresis = "gateHysteresis";

    // Signal path
    inline constexpr const char* glide    = "glide";
    inline constexpr const char* dryLevel = "dryLevel";

    // OSC 1
    inline constexpr const char* waveform     = "waveform";
    inline constexpr const char* oscLevel     = "oscLevel";
    inline constexpr const char* unisonVoices = "unisonVoices";
    inline constexpr const char* unisonDetune = "unisonDetune";
    inline constexpr const char* unisonBlend  = "unisonBlend";
    inline constexpr const char* octaveShift  = "octaveShift";
    inline constexpr const char* coarseTune   = "coarseTune";
    inline constexpr const char* fineTune     = "fineTune";
    inline constexpr const char* morph        = "morph";

    // Sub oscillator
    inline constexpr const char* subLevel        = "subLevel";
    inline constexpr const char* subOctave       = "subOctave";
    inline constexpr const char* subBypassFilter = "subBypassFilter";

    // OSC 2
    inline constexpr const char* osc2Waveform     = "osc2Waveform";
    inline constexpr const char* osc2Level        = "osc2Level";
    inline constexpr const char* osc2UnisonVoices = "osc2UnisonVoices";
    inline constexpr const char* osc2UnisonDetune = "osc2UnisonDetune";
    inline constexpr const char* osc2UnisonBlend  = "osc2UnisonBlend";
    inline constexpr const char* osc2OctaveShift  = "osc2OctaveShift";
    inline constexpr const char* osc2CoarseTune   = "osc2CoarseTune";
    inline constexpr const char* osc2FineTune     = "osc2FineTune";
    inline constexpr const char* osc2Morph        = "osc2Morph";

    // Filter
    inline constexpr const char* envResonance = "envResonance";
    inline constexpr const char* filterType   = "filterType";
    inline constexpr const char* freqTracking = "freqTracking";
    inline constexpr const char* filterFreq   = "filterFreq";

    // Output
    inline constexpr const char* masterVolume  = "masterVolume";
    inline constexpr const char* ampEnvSource  = "ampEnvSource";

    // LFO
    inline constexpr const char* lfoRate   = "lfoRate";
    inline constexpr const char* lfoShape  = "lfoShape";
    inline constexpr const char* lfoAmount = "lfoAmount";

    // Transient
    inline constexpr const char* transientSlope  = "transientSlope";
    inline constexpr const char* transientPitch  = "transientPitch";
    inline constexpr const char* transientLevel  = "transientLevel";
    inline constexpr const char* transientAttack = "transientAttack";
    inline constexpr const char* transientDecay  = "transientDecay";

    // Mod envelope
    inline constexpr const char* modEnvAttack = "modEnvAttack";
    inline constexpr const char* modEnvDecay  = "modEnvDecay";

} // namespace ParamIDs
