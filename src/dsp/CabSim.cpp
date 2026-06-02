#include "CabSim.h"
#include <cassert>

void CabSim::OnReset(double sampleRate)
{
    mBuf.fill(0.f);
    mWritePos     = 0;
    mCrossfadePos = 0;
    mEnabled      = mTargetEnabled;
    CalcHighShelf(mHighShelf, static_cast<float>(sampleRate), kHSFreq, kHSGain, kHSQ);
    mHighShelf.Clear();
}

// Audio EQ Cookbook — high-shelf
void CabSim::CalcHighShelf(Biquad& f, float sr, float freq, float gainDb, float q)
{
    const float A     = std::pow(10.f, gainDb / 40.f);
    const float w0    = 2.f * kPi * freq / sr;
    const float cosW  = std::cos(w0);
    const float sinW  = std::sin(w0);
    const float sqrtA = std::sqrt(A);
    const float alpha = (sinW / 2.f) * std::sqrt((A + 1.f/A) * (1.f/q - 1.f) + 2.f);

    const float b0 =     A * ((A+1.f) + (A-1.f)*cosW + 2.f*sqrtA*alpha);
    const float b1 = -2.f*A * ((A-1.f) + (A+1.f)*cosW);
    const float b2 =     A * ((A+1.f) + (A-1.f)*cosW - 2.f*sqrtA*alpha);
    const float a0 =          (A+1.f) - (A-1.f)*cosW + 2.f*sqrtA*alpha;
    const float a1 =  2.f *  ((A-1.f) - (A+1.f)*cosW);
    const float a2 =          (A+1.f) - (A-1.f)*cosW - 2.f*sqrtA*alpha;

    f.b0 = b0 / a0;
    f.b1 = b1 / a0;
    f.b2 = b2 / a0;
    f.a1 = a1 / a0;
    f.a2 = a2 / a0;
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

        wet = mHighShelf.Process(wet);

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
