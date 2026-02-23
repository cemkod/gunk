#pragma once
#include "Oscillator.h"

// Linear pitch-ramp state machine.  Mirrors the FilterEngine.h pattern.
struct GlideEngine
{
    // Accessible to the processor for passing to the envelope filter
    float lastDetectedFreq = 0.0f;

    void reset()
    {
        lastDetectedFreq    = 0.0f;
        glideFreq           = 0.0f;
        glideSourceFreq     = 0.0f;
        glideTargetFreq     = 0.0f;
        glideSamplesElapsed = 0;
        glideSamplesTotal   = 0;
        glideSnapHops       = 0;
    }

    void update (float detectedFreq, int glideSamples, bool gateIsOpen,
                 WavetableOscillator& oscillator, WavetableOscillator& subOscillator,
                 double currentSampleRate)
    {
        if (detectedFreq > 0.0f)
        {
            lastDetectedFreq = detectedFreq;

            if (glideFreq < 1.0f || glideSnapHops > 0)
            {
                // Cold start or detector still settling: snap directly, no ramp
                glideFreq       = detectedFreq;
                glideSourceFreq = detectedFreq;
                glideTargetFreq = detectedFreq;
                glideSamplesElapsed = 0;
                glideSamplesTotal   = 0;
                if (glideSnapHops > 0)
                    --glideSnapHops;
                else
                    glideSnapHops = kSnapHopsOnInit; // snap for next N detections while detector settles
            }
            else if (std::abs (detectedFreq - glideTargetFreq) > kFreqChangeTolerance)
            {
                // New pitch target: start a new linear ramp
                glideSourceFreq     = glideFreq;
                glideTargetFreq     = detectedFreq;
                glideSamplesElapsed = 0;
                glideSamplesTotal   = glideSamples;
            }

            // Advance the linear ramp
            if (glideSamplesTotal > 0 && glideSamplesElapsed < glideSamplesTotal)
            {
                const float t = (float) ++glideSamplesElapsed / (float) glideSamplesTotal;
                glideFreq = glideSourceFreq + (glideTargetFreq - glideSourceFreq) * t;
            }
            else
            {
                glideFreq = glideTargetFreq;
            }

            oscillator.setFrequency (glideFreq, currentSampleRate);
            subOscillator.setFrequency (glideFreq / 2.0f, currentSampleRate);
        }
        else if (!gateIsOpen || lastDetectedFreq < 1.0f)
        {
            lastDetectedFreq    = 0.0f;
            glideFreq           = 0.0f;
            glideSourceFreq     = 0.0f;
            glideTargetFreq     = 0.0f;
            glideSamplesElapsed = 0;
            glideSamplesTotal   = 0;
            glideSnapHops       = 0;
            oscillator.reset();
            subOscillator.reset();
        }
        // else: detection lost but gate still open — hold last frequency
    }

private:
    static constexpr int   kSnapHopsOnInit       = 4;
    static constexpr float kFreqChangeTolerance  = 0.001f;

    float glideFreq           = 0.0f;
    float glideSourceFreq     = 0.0f;
    float glideTargetFreq     = 0.0f;
    int   glideSamplesElapsed = 0;
    int   glideSamplesTotal   = 0;
    int   glideSnapHops       = 0; // snap (no ramp) for first N detections after gate-open
};
