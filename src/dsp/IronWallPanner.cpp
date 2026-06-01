#include <cassert>
#include "IronWallPanner.h"
#include <cmath>
#include <algorithm>

void IronWallPanner::OnReset(double sampleRate)
{
    assert(sampleRate > 0.0);

    mDelaySamples = static_cast<int>(sampleRate * 0.007);
    mBufSize      = mDelaySamples + 64;
    mBuf.assign(mBufSize, 0.f);
    std::fill(mBuf.begin(), mBuf.end(), 0.f);

    mWritePos    = 0;
    mReadPos     = static_cast<float>(mBufSize - mDelaySamples); // circular gap = mDelaySamples

    mCorrelation = 1.f;
    mWarning     = false;
    mSumLR       = 0.f;
    mSumLL       = 0.f;
    mSumRR       = 0.f;
    mCorrCounter = 0;
}

void IronWallPanner::SetWidth(float width)
{
    mWidth = width;
}

float IronWallPanner::GetCorrelation() const
{
    return mCorrelation;
}

bool IronWallPanner::GetWarning() const
{
    return mWarning;
}

void IronWallPanner::ProcessBlock(float* mono, float* outL, float* outR, int nFrames)
{
    assert(mBufSize > 0);
    const float bufSizeF = static_cast<float>(mBufSize);

    for (int i = 0; i < nFrames; ++i)
    {
        const float in = mono[i];

        // Write into circular buffer — mWritePos stays in [0, mBufSize) to prevent
        // float precision loss in the drift check (float exact only up to 2^24 ≈ 6 min at 44100 Hz)
        mBuf[mWritePos] = in;
        mWritePos = (mWritePos + 1) % mBufSize;

        // Fractional read for +2 cents pitch shift
        const int   idx0    = static_cast<int>(mReadPos) % mBufSize;
        const int   idx1    = (idx0 + 1) % mBufSize;
        const float frac    = mReadPos - std::floor(mReadPos);
        const float delayed = mBuf[idx0] + frac * (mBuf[idx1] - mBuf[idx0]);

        mReadPos += kPitchRatio;
        if (mReadPos >= bufSizeF)
            mReadPos -= bufSizeF; // keep in [0, mBufSize) — same precision guarantee

        // Drift reset: kPitchRatio > 1.0 means read gains on write over time.
        // Circular gap = (write - read + bufSize) % bufSize.
        // Re-anchor when gap closes to < 2 samples.
        const float gap = std::fmod(static_cast<float>(mWritePos) - mReadPos + bufSizeF, bufSizeF);
        if (gap < 2.f)
            mReadPos = std::fmod(static_cast<float>(mWritePos) - static_cast<float>(mDelaySamples) + bufSizeF, bufSizeF);

        const float L = in;
        const float R = in + mWidth * (delayed - in); // lerp(in, delayed, mWidth)

        outL[i] = L;
        outR[i] = R;

        // Correlation accumulation
        mSumLR += L * R;
        mSumLL += L * L;
        mSumRR += R * R;
        mCorrCounter++;

        if (mCorrCounter == kCorrelationWindow)
        {
            mCorrelation = mSumLR / std::sqrt(mSumLL * mSumRR + 1e-12f);
            mCorrelation = std::clamp(mCorrelation, -1.f, 1.f);
            mWarning     = (mCorrelation < 0.5f);

            mSumLR       = 0.f;
            mSumLL       = 0.f;
            mSumRR       = 0.f;
            mCorrCounter = 0;
        }
    }
}
