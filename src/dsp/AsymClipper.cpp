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

void AsymClipper::ProcessBlock(sample* in, sample* out, int nFrames)
{
    // Wrap raw pointers into T** as required by OverSampler::ProcessBlock.
    // No heap allocation — stack pointers only.
    sample* inPtrs[1]  { in  };
    sample* outPtrs[1] { out };

    // Lambda: receives a block at 4x SR, clips, and writes to oversampler's
    // output buffer. No captures that cause malloc — only the class constants
    // (which are constexpr statics) are used.
    //
    // NOTE: std::function with captures CAN allocate — the iPlug2 comment in
    // Oversampler.h warns about this. This lambda has NO captures to avoid it.
    mOverSampler.ProcessBlock(
        inPtrs,
        outPtrs,
        nFrames,
        /*nInChans=*/1,
        /*nOutChans=*/1,
        [](sample** inputs, sample** outputs, int frames)
        {
            const sample* src = inputs[0];
            sample*       dst = outputs[0];

            for (int i = 0; i < frames; ++i)
            {
                dst[i] = static_cast<sample>(AsymClipper::Clip(static_cast<float>(src[i])));
            }
        }
    );

    // Apply gain compensation to normalise perceived level after hard limiting.
    // kGainComp is constexpr — no branching, no memory access in hot path.
    for (int i = 0; i < nFrames; ++i)
    {
        out[i] *= static_cast<sample>(kGainComp);
    }
}

int AsymClipper::GetLatency() const
{
    return mLatency;
}

END_IPLUG_NAMESPACE
