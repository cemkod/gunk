#pragma once
#include <algorithm>
#include <cmath>

struct ADSREnvelope
{
    enum class Phase { Idle, Attack, Decay, Sustain, Release };

    struct Coefficients
    {
        float attCoef  = 0.0f;
        float decCoef  = 0.0f;
        float susLevel = 0.0f;
        float relCoef  = 0.0f;
    };

    void reset()
    {
        phase              = Phase::Idle;
        value              = 0.0f;
        velocity           = 1.0f;
        velocityTarget     = 1.0f;
        peakTrackRemaining = 0;
    }

    // Call once per sample inside the audio loop.
    float update (bool noteOn, bool noteOff, const Coefficients& coef)
    {
        if (noteOn)
            phase = Phase::Attack;
        if (noteOff && phase != Phase::Idle)
            phase = Phase::Release;

        switch (phase)
        {
            case Phase::Attack:
                value += coef.attCoef * (1.0f - value);
                if (value >= 0.999f) { value = 1.0f; phase = Phase::Decay; }
                break;
            case Phase::Decay:
                value += coef.decCoef * (coef.susLevel - value);
                if (std::abs (value - coef.susLevel) < 0.001f)
                    phase = Phase::Sustain;
                break;
            case Phase::Sustain:
                value = coef.susLevel;
                break;
            case Phase::Release:
                value += coef.relCoef * (0.0f - value);
                if (value < 0.001f) { value = 0.0f; phase = Phase::Idle; }
                break;
            case Phase::Idle:
            default:
                value = 0.0f;
                break;
        }
        return value;
    }

    // Call once per sample. Tracks the input peak in a 100ms window after onset, and
    // smooths the result (~5ms one-pole) so the envelope never steps as the peak rises.
    void trackVelocity (bool noteOn, float inputSampleAbs, double sampleRate)
    {
        if (sampleRate != smoothRate)
        {
            smoothRate = sampleRate;
            smoothCoef = 1.0f - (float) std::exp (-1.0 / (0.005 * sampleRate));
        }

        if (noteOn)
        {
            // A retrigger inside the window (gate-open then aubio onset on the same
            // note) keeps the running peak instead of resetting it to one raw sample.
            velocityTarget     = peakTrackRemaining > 0 ? std::max (velocityTarget, inputSampleAbs)
                                                        : inputSampleAbs;
            peakTrackRemaining = (int) (sampleRate * 0.1);
        }
        else if (peakTrackRemaining > 0)
        {
            --peakTrackRemaining;
            if (inputSampleAbs > velocityTarget)
                velocityTarget = inputSampleAbs;
        }

        velocity += smoothCoef * (velocityTarget - velocity);
    }

    float getVelocity() const { return velocity; }
    Phase getPhase()    const { return phase; }

private:
    Phase phase              = Phase::Idle;
    float value              = 0.0f;
    float velocity           = 1.0f;
    float velocityTarget     = 1.0f;
    int   peakTrackRemaining = 0;
    double smoothRate        = 0.0;
    float  smoothCoef        = 1.0f;
};
