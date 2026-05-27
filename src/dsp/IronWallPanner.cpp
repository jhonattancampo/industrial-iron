#include <cassert>
#include "IronWallPanner.h"
#include <cmath>
#include <algorithm>

void IronWallPanner::OnReset(double sampleRate)
{
    assert(sampleRate > 0.0);

    mDelayL   = static_cast<int>(std::ceil(sampleRate * 0.0035));
    mDelayR   = static_cast<int>(std::ceil(sampleRate * 0.007));
    mBufSizeL = mDelayL + 64;
    mBufSizeR = mDelayR + 64;

    mBufL.assign(mBufSizeL, 0.f);
    mBufR.assign(mBufSizeR, 0.f);

    mWritePosL = mDelayL;
    mWritePosR = mDelayR;
    mReadPosR  = 0.f;

    mCorrelation = 1.f;
    mWarning     = false;
    mSumLR = mSumLL = mSumRR = 0.f;
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
    assert(mBufSizeL > 0 && mBufSizeR > 0);

    for (int i = 0; i < nFrames; ++i)
    {
        const float in = mono[i];

        // --- L: 3.5ms clean delay (integer read, no pitch) ---
        mBufL[mWritePosL % mBufSizeL] = in;
        const float delayedL = mBufL[(mWritePosL - mDelayL + mBufSizeL) % mBufSizeL];
        mWritePosL++;

        // --- R: 7ms + 2-cent pitch shift ---
        mBufR[mWritePosR % mBufSizeR] = in;
        mWritePosR++;

        const int   idx0     = static_cast<int>(mReadPosR) % mBufSizeR;
        const int   idx1     = (idx0 + 1) % mBufSizeR;
        const float frac     = mReadPosR - std::floor(mReadPosR);
        const float delayedR = mBufR[idx0] + frac * (mBufR[idx1] - mBufR[idx0]);

        mReadPosR += kPitchRatio;
        if ((static_cast<float>(mWritePosR) - mReadPosR) < 2.f)
            mReadPosR -= static_cast<float>(mDelayR);

        // Width crossfade — delayedL as base so width=0 → centered mono at 3.5ms
        const float L = delayedL;
        const float R = delayedL + mWidth * (delayedR - delayedL);

        outL[i] = L;
        outR[i] = R;

        // Correlation accumulation (unchanged)
        mSumLR += L * R;
        mSumLL += L * L;
        mSumRR += R * R;
        mCorrCounter++;

        if (mCorrCounter == kCorrelationWindow)
        {
            mCorrelation = mSumLR / std::sqrt(mSumLL * mSumRR + 1e-12f);
            mCorrelation = std::clamp(mCorrelation, -1.f, 1.f);
            mWarning     = (mCorrelation < 0.5f);

            mSumLR = mSumLL = mSumRR = 0.f;
            mCorrCounter = 0;
        }
    }
}
