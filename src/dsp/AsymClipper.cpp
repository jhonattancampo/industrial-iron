#include "AsymClipper.h"

#include <cmath>

BEGIN_IPLUG_NAMESPACE

// OverSampler latency at x4 (HIIR polyphase half-band FIR filters):
//
//   Stage 1 up  (2x, 12 coeff) : group delay ≈ 12 samples at 2x SR  → 6 at 1x SR
//   Stage 2 up  (2x,  4 coeff) : group delay ≈  4 samples at 4x SR  → 1 at 1x SR
//   Stage 2 down(2x,  4 coeff) : group delay ≈  4 samples at 4x SR  → 1 at 1x SR
//   Stage 1 down(2x, 12 coeff) : group delay ≈ 12 samples at 2x SR  → 6 at 1x SR
//
//   Total                                                            ≈ 14 samples
//
// This is a fixed integer — sample rate does not change it (it is expressed in
// original-rate samples, and the filter order is constant).
static constexpr int kOversamplerLatency = 14;

// ---------------------------------------------------------------------------

AsymClipper::AsymClipper()
{
    // Member is constructed with k4x in the header initialiser — nothing extra needed.
}

void AsymClipper::OnReset(double sampleRate)
{
    mOverSampler.Reset();
    mLatency = kOversamplerLatency;

    mAttackCoeff  = 1.f - std::exp(-2.2f / static_cast<float>(0.001 * sampleRate));
    mReleaseCoeff = 1.f - std::exp(-2.2f / static_cast<float>(0.050 * sampleRate));
    mEnv = 0.f;
}

// ---------------------------------------------------------------------------
// Stage implementations
// ---------------------------------------------------------------------------

// Stage 1 — Triode-approximate waveshaper.
// Asymmetric: positive half uses full drive, negative uses 0.8× drive.
// This produces even harmonics (second harmonic dominant) — valve character.
// Small-signal gain = drive, so unity gain at small amplitudes when drive = 1.0.
float AsymClipper::TubeSat(float x, float drive) noexcept
{
    if (x >= 0.f)
        return  1.f - std::exp(-x * drive * 0.9f);
    else
        return -1.f + std::exp( x * 0.65f * drive);
}

// Stage 2 — Asymmetric hard clip (original character preserved).
// Positive threshold 0.70, negative 0.85 — intentional asymmetry.
float AsymClipper::HardClip(float x) noexcept
{
    if (x >  kPosThreshold) return  kPosThreshold;
    if (x < -kNegThreshold) return -kNegThreshold;
    return x;
}

// Stage 3 — Soft output shaper.
// tanh(x * 1.2) / tanh(1.2) normalises the curve so output peak = 1.0
// at input = 1.0. Rounds hard-clip transient artifacts without adding
// significant level loss for moderate amplitudes.
float AsymClipper::OutputShape(float x) noexcept
{
    return std::tanh(x * kTanhShape) / std::tanh(kTanhShape);
}

// ---------------------------------------------------------------------------

void AsymClipper::ProcessBlock(sample* in, sample* out, int nFrames)
{
    // Wrap raw pointers into T** as required by OverSampler::ProcessBlock.
    // No heap allocation — stack pointers only.
    sample* inPtrs[1]  { in  };
    sample* outPtrs[1] { out };

    // Envelope follower at original rate — feeds subtle ±10% drive modulation.
    // One-pole attack/release follower; coefficients computed in OnReset().
    for (int s = 0; s < nFrames; ++s)
    {
        const float level = std::abs(static_cast<float>(in[s]));
        const float coeff = (level > mEnv) ? mAttackCoeff : mReleaseCoeff;
        mEnv += coeff * (level - mEnv);
    }
    const float envDriveMod = 1.0f + 0.10f * mEnv;

    // Snapshot drive + envDriveMod for lambda capture (both are plain floats,
    // well within SBO — no heap allocation).
    const float driveSnap = mDrive;

    // All three stages run inside the oversampler lambda at 4× SR.
    // DC bias shifts the waveform asymmetrically to generate even harmonics
    // before the waveshaper, then a proportional offset is removed after.
    mOverSampler.ProcessBlock(
        inPtrs,
        outPtrs,
        nFrames,
        /*nInChans=*/1,
        /*nOutChans=*/1,
        [driveSnap, envDriveMod](sample** inputs, sample** outputs, int frames)
        {
            const sample* src = inputs[0];
            sample*       dst = outputs[0];

            for (int i = 0; i < frames; ++i)
            {
                float x = static_cast<float>(src[i]);
                x += 0.025f;                               // 4a: DC bias in
                x = TubeSat(x, driveSnap * envDriveMod);  // Stage 1: even harmonics, valve warmth
                x = HardClip(x);                           // Stage 2: asymmetric hard limit
                x = OutputShape(x);                        // Stage 3: smooth hard-clip artifacts
                x -= 0.025f * 0.3f;                        // 4a: DC bias out
                dst[i] = static_cast<sample>(x);
            }
        }
    );

    // Makeup gain: compensates perceived level reduction through the 3-stage chain.
    // kMakeupGain = tanh(1.2) / tanh(kPosThreshold * 1.2) ≈ 1.218
    for (int i = 0; i < nFrames; ++i)
        out[i] *= static_cast<sample>(kMakeupGain);
}

int AsymClipper::GetLatency() const
{
    return mLatency;
}

END_IPLUG_NAMESPACE
