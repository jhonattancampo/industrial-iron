#include "CabSim.h"
#include <cassert>

void CabSim::OnReset(double /*sampleRate*/)
{
    mBuf.fill(0.f);
    mWritePos   = 0;
    mCrossfadePos = 0;
    mEnabled    = mTargetEnabled;
}

void CabSim::SetEnabled(bool enabled)
{
    if (enabled == mTargetEnabled)
        return;
    mTargetEnabled = enabled;
    mFadingIn      = enabled;
    mCrossfadePos  = kCrossfadeSamples;
}

bool CabSim::GetEnabled() const { return mTargetEnabled; }

void CabSim::SetCabType(int type)
{
    switch (type)
    {
        case 0:  mActiveIR = kIR_Marshall; break;
        case 1:  mActiveIR = kIR_Mesa;     break;
        default: mActiveIR = kIR_V30;      break;
    }
}

void CabSim::ProcessBlock(float* in, float* out, int nFrames)
{
    assert(in && out && nFrames > 0);
    const float* ir = mActiveIR;

    for (int i = 0; i < nFrames; ++i)
    {
        const float dry = in[i];

        mBuf[mWritePos] = dry;

        float wet = 0.f;
        for (int n = 0; n < kIRLen; ++n)
            wet += ir[n] * mBuf[(mWritePos + kIRLen - n) & kMask];

        mWritePos = (mWritePos + 1) & kMask;

        float result;
        if (mCrossfadePos > 0)
        {
            const float t       = static_cast<float>(kCrossfadeSamples - mCrossfadePos + 1)
                                  / static_cast<float>(kCrossfadeSamples);
            const float wetGain = mFadingIn ? t : (1.f - t);
            result = dry * (1.f - wetGain) + wet * wetGain;
            if (--mCrossfadePos == 0)
                mEnabled = mTargetEnabled;
        }
        else
        {
            result = mEnabled ? wet : dry;
        }

        out[i] = result;
    }
}
