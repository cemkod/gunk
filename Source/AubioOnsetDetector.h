#pragma once
#include <aubio/aubio.h>

class AubioOnsetDetector
{
public:
    static constexpr uint_t kBufSize = 2048;
    static constexpr uint_t kHopSize = 128; // ~2.9ms at 44.1kHz

    void prepare (double sampleRate)
    {
        cleanup();
        sr  = static_cast<uint_t> (sampleRate);
        in  = new_fvec (kHopSize);
        out = new_fvec (1);
        // "energy" = spectral energy ODF: no adaptive whitening, no log
        // compression, instant convergence. "complex" looked good on paper but
        // its mandatory adaptive spectral whitening (~250ms convergence) meant
        // it produced near-zero ODF values until warm-up finished, causing the
        // barely-detects-anything symptom that threshold tuning cannot fix.
        onset = new_aubio_onset ("energy", kBufSize, kHopSize, sr);
        aubio_onset_set_minioi_ms (onset, 150.0f); // 20ms min inter-onset
        hopPos = 0;

    }

    // Returns true on the frame that an onset is detected.
    // Call once per sample on the audio thread.
    bool processSample (float sample)
    {
        fvec_set_sample (in, sample, hopPos);
        if (++hopPos < kHopSize)
            return false;

        hopPos = 0;
        aubio_onset_do (onset, in, out);
        return fvec_get_sample (out, 0) > 0.0f;
    }

    void setThreshold (float t)
    {
        if (onset) aubio_onset_set_threshold (onset, t);
    }

    void reset()
    {
        if (onset) aubio_onset_reset (onset);
        hopPos = 0;
    }

    ~AubioOnsetDetector() { cleanup(); }

private:
    void cleanup()
    {
        if (onset) { del_aubio_onset (onset); onset = nullptr; }
        if (in)    { del_fvec (in);           in    = nullptr; }
        if (out)   { del_fvec (out);          out   = nullptr; }
    }

    aubio_onset_t* onset  = nullptr;
    fvec_t*        in     = nullptr;
    fvec_t*        out    = nullptr;
    uint_t         sr     = 44100;
    uint_t         hopPos = 0;
};
