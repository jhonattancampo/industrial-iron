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

    mWritePos    = mDelaySamples;
    mReadPos     = 0.f;

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

    for (int i = 0; i < nFrames; ++i)
    {
        const float in = mono[i];

        // Write into circular buffer
        mBuf[mWritePos % mBufSize] = in;
        mWritePos++;

        // Fractional read for +2 cents pitch shift
        const int   idx0    = static_cast<int>(mReadPos) % mBufSize;
        const int   idx1    = (idx0 + 1) % mBufSize;
        const float frac    = mReadPos - std::floor(mReadPos);
        const float delayed = mBuf[idx0] + frac * (mBuf[idx1] - mBuf[idx0]);

        mReadPos += kPitchRatio;

        // Drift reset: kPitchRatio > 1.0 means mReadPos closes on mWritePos over time.
        // Re-anchor by subtracting mDelaySamples so the read pointer stays ~7ms behind.
        if ((static_cast<float>(mWritePos) - mReadPos) < 2.f)
            mReadPos -= static_cast<float>(mDelaySamples);

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
