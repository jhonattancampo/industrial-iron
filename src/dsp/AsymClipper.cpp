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

void AsymClipper::OnReset(double /*sampleRate*/)
{
    // Reset oversampler internal filter state (clears any stale samples between
    // host play/stop cycles). DEFAULT_BLOCK_SIZE is fine for buffer pre-allocation;
    // the oversampler will resize automatically on the first ProcessBlock call if
    // nFrames differs.
    mOverSampler.Reset();

    // Latency is filter-order-based, not sample-rate-dependent.
    mLatency = kOversamplerLatency;
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
        return  1.f - std::exp(-x * drive);
    else
        return -1.f + std::exp( x * 0.8f * drive);
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

    // Snapshot drive for the lambda capture.
    // A single float (4 bytes) is well within std::function SBO on all major
    // implementations (libc++ ≥ 24 bytes, libstdc++ ≥ 16 bytes) — no heap alloc.
    const float driveSnap = mDrive;

    // All three stages run inside the oversampler lambda at 4× SR.
    // Harmonic generation from TubeSat and OutputShape is therefore fully
    // covered by the anti-aliasing filters — no separate oversampling needed.
    mOverSampler.ProcessBlock(
        inPtrs,
        outPtrs,
        nFrames,
        /*nInChans=*/1,
        /*nOutChans=*/1,
        [driveSnap](sample** inputs, sample** outputs, int frames)
        {
            const sample* src = inputs[0];
            sample*       dst = outputs[0];

            for (int i = 0; i < frames; ++i)
            {
                float x = static_cast<float>(src[i]);
                x = TubeSat(x, driveSnap);  // Stage 1: even harmonics, valve warmth
                x = HardClip(x);             // Stage 2: asymmetric hard limit
                x = OutputShape(x);          // Stage 3: smooth hard-clip artifacts
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
