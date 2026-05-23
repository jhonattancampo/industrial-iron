#pragma once

#include <cassert>
#include "IPlugConstants.h"
#include "Extras/Oversampler.h"

BEGIN_IPLUG_NAMESPACE

/**
 * AsymClipper — hard asymmetric clipper with mandatory x4 oversampling.
 *
 * Positive threshold : kPosThreshold (0.70)
 * Negative threshold : kNegThreshold (0.85)
 * Oversampling       : x4 (EFactor::k4x) — rejection is automatic without it
 *
 * Usage:
 *   OnReset(sampleRate) — call from the plugin's OnReset()
 *   ProcessBlock(in, out, nFrames) — call per-channel from the plugin's ProcessBlock()
 *   GetLatency() — returns latency in original-rate samples (report to host)
 */
class AsymClipper
{
public:
    // -----------------------------------------------------------------------
    // Named constants — QA calibration lives here, not in the implementation
    // -----------------------------------------------------------------------
    static constexpr float kPosThreshold = 0.70f;
    static constexpr float kNegThreshold = 0.85f;
    static constexpr float kGainComp     = 1.0f / ((kPosThreshold + kNegThreshold) * 0.5f);

    AsymClipper();

    /** Call from the plugin's OnReset(). Resets filter state and caches latency. */
    void OnReset(double sampleRate);

    /**
     * Process one channel of audio.
     * @param in      Input buffer  (nFrames samples)
     * @param out     Output buffer (nFrames samples, may alias in)
     * @param nFrames Number of samples
     */
    void ProcessBlock(sample* in, sample* out, int nFrames);

    /** Latency introduced by the x4 oversampler, in original-rate samples. */
    int GetLatency() const;

    /** Hard-clip a single sample. Public so the no-capture lambda in ProcessBlock can call it. */
    static float Clip(float x) noexcept
    {
        if (x >  kPosThreshold) return  kPosThreshold;
        if (x < -kNegThreshold) return -kNegThreshold;
        return x;
    }

private:

    // x4 oversampler — mono (1-in / 1-out), block-processing mode
    OverSampler<sample> mOverSampler { EFactor::k4x, /*blockProcessing=*/true, 1, 1 };

    int mLatency = 0;
};

END_IPLUG_NAMESPACE
