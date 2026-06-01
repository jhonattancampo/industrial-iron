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
    mReadPos     = mBufSize - mDelaySamples; // lags write by exactly mDelaySamples

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

        mBuf[mWritePos] = in;
        mWritePos = (mWritePos + 1) % mBufSize;

        const float delayed = mBuf[mReadPos];
        mReadPos = (mReadPos + 1) % mBufSize;

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
