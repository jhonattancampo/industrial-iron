#pragma once

#include <cassert>
#include <cmath>
#include "IPlugConstants.h"
#include "Extras/Oversampler.h"

BEGIN_IPLUG_NAMESPACE

/**
 * AsymClipper — 3-stage distortion chain with mandatory x4 oversampling.
 *
 * Stage 1: TubeSat  — asymmetric soft waveshaper (triode approximation)
 * Stage 2: HardClip — asymmetric hard clip (pos: 0.70, neg: 0.85)
 * Stage 3: OutputShape — tanh soft limiter, rounds hard-clip artifacts
 *
 * All three stages run inside the x4 oversampler — full aliasing rejection.
 *
 * Usage:
 *   OnReset(sampleRate) — call from the plugin's OnReset()
 *   SetDrive(drive)     — call once per block before ProcessBlock(); drive ∈ [1.0, 5.0]
 *   ProcessBlock(in, out, nFrames)
 *   GetLatency()        — returns latency in original-rate samples (report to host)
 */
class AsymClipper
{
public:
    // -----------------------------------------------------------------------
    // Named constants — QA calibration lives here, not in the implementation
    // -----------------------------------------------------------------------
    static constexpr float kPosThreshold = 0.70f;
    static constexpr float kNegThreshold = 0.85f;
    static constexpr float kTanhShape    = 1.2f;

    // Makeup gain compensates Stage 3 output reduction.
    // Stage 3 peak output for a signal clipped at kPosThreshold:
    //   tanh(0.70 * 1.2) / tanh(1.2) = tanh(0.84) / tanh(1.2) ≈ 0.821
    // kMakeupGain = tanh(1.2) / tanh(0.84) ≈ 1.218
    static constexpr float kMakeupGain = 1.218f;

    AsymClipper();

    /** Call from the plugin's OnReset(). Resets filter state and caches latency. */
    void OnReset(double sampleRate);

    /**
     * Set tube saturation drive. Call once per block before ProcessBlock().
     * drive ∈ [1.0, 5.0]: 1.0 = subtle warmth, 5.0 = full saturation.
     * At drive=1.0 the waveshaper is unity-gain for small amplitudes.
     */
    void SetDrive(float drive) noexcept { mDrive = drive; }

    /**
     * Process one channel of audio through the 3-stage distortion chain.
     * @param in      Input buffer  (nFrames samples)
     * @param out     Output buffer (nFrames samples, may alias in)
     * @param nFrames Number of samples
     */
    void ProcessBlock(sample* in, sample* out, int nFrames);

    /** Latency introduced by the x4 oversampler, in original-rate samples. */
    int GetLatency() const;

private:
    // Stage 1 — triode-approximate waveshaper, asymmetric (even harmonics)
    static float TubeSat(float x, float drive) noexcept;

    // Stage 2 — asymmetric hard clip (preserves existing character)
    static float HardClip(float x) noexcept;

    // Stage 3 — soft output shaper, rounds hard-clip transient artifacts
    static float OutputShape(float x) noexcept;

    // x4 oversampler — mono (1-in / 1-out), block-processing mode
    OverSampler<sample> mOverSampler { EFactor::k4x, /*blockProcessing=*/true, 1, 1 };

    float mDrive        = 1.0f;
    int   mLatency      = 0;
    float mEnv          = 0.f;
    float mAttackCoeff  = 0.f;
    float mReleaseCoeff = 0.f;
};

END_IPLUG_NAMESPACE
