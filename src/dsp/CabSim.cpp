#include <cassert>
#include "CabSim.h"

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void CabSim::OnReset(double sampleRate)
{
    assert(sampleRate > 0.0);
    mSampleRate = sampleRate;

    CalcLowShelf(mLowShelf, sampleRate, kLowShelfFreq, kLowShelfGain);
    CalcPeaking (mBoxRes,   sampleRate, kBoxResFreq,   kBoxResQ,  kBoxResGain);
    CalcLowPass (mHighCut,  sampleRate, kHighCutFreq,  kHighCutQ);
    CalcLowPass (mAirCut,   sampleRate, kAirCutFreq,   kAirCutQ);

    mLowShelf.ClearState();
    mBoxRes.ClearState();
    mHighCut.ClearState();
    mAirCut.ClearState();
}

void CabSim::SetEnabled(bool enabled)
{
    if (enabled == mTargetEnabled)
        return;

    mTargetEnabled  = enabled;
    mFadingIn       = enabled;   // true → ramp wet up; false → ramp wet down
    mCrossfadePos   = kCrossfadeSamples;
}

bool CabSim::GetEnabled() const
{
    return mTargetEnabled;
}

void CabSim::ProcessBlock(float* in, float* out, int nFrames)
{
    assert(in != nullptr && out != nullptr && nFrames > 0);

    for (int i = 0; i < nFrames; ++i)
    {
        const double dry = static_cast<double>(in[i]);

        // Run the 4-stage biquad cascade regardless of bypass state so that
        // filter state stays warm and the crossfade has something to mix with.
        double wet = mLowShelf.Process(dry);
        wet        = mBoxRes.Process(wet);
        wet        = mHighCut.Process(wet);
        wet        = mAirCut.Process(wet);

        double result;

        if (mCrossfadePos > 0)
        {
            // Linear ramp: progress goes from kCrossfadeSamples down to 1
            // t moves from near-0 to near-1 as the fade advances.
            double t = static_cast<double>(kCrossfadeSamples - mCrossfadePos + 1)
                       / static_cast<double>(kCrossfadeSamples);

            double wetGain = mFadingIn ? t : (1.0 - t);
            result = dry * (1.0 - wetGain) + wet * wetGain;

            --mCrossfadePos;
            if (mCrossfadePos == 0)
                mEnabled = mTargetEnabled;
        }
        else if (mEnabled)
        {
            result = wet;
        }
        else
        {
            result = dry;
        }

        out[i] = static_cast<float>(result);
    }
}

// ---------------------------------------------------------------------------
// Coefficient calculators (Audio EQ Cookbook formulas)
// ---------------------------------------------------------------------------

void CabSim::CalcLowShelf(Biquad& f, double sr, double freq, double gainDb)
{
    double A     = std::sqrt(std::pow(10.0, gainDb / 20.0));
    double w0    = 2.0 * kPi * freq / sr;
    double cosW  = std::cos(w0);
    double sinW  = std::sin(w0);
    double S     = 1.0;
    double alpha = (sinW / 2.0) * std::sqrt((A + 1.0 / A) * (1.0 / S - 1.0) + 2.0);
    double sqrtA = std::sqrt(A);

    double b0 =  A * ((A + 1.0) - (A - 1.0) * cosW + 2.0 * sqrtA * alpha);
    double b1 =  2.0 * A * ((A - 1.0) - (A + 1.0) * cosW);
    double b2 =  A * ((A + 1.0) - (A - 1.0) * cosW - 2.0 * sqrtA * alpha);
    double a0 =       (A + 1.0) + (A - 1.0) * cosW + 2.0 * sqrtA * alpha;
    double a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cosW);
    double a2 =        (A + 1.0) + (A - 1.0) * cosW - 2.0 * sqrtA * alpha;

    f.b0 = b0 / a0;
    f.b1 = b1 / a0;
    f.b2 = b2 / a0;
    f.a1 = a1 / a0;
    f.a2 = a2 / a0;
}

void CabSim::CalcPeaking(Biquad& f, double sr, double freq, double q, double gainDb)
{
    double A     = std::sqrt(std::pow(10.0, gainDb / 20.0));
    double w0    = 2.0 * kPi * freq / sr;
    double alpha = std::sin(w0) / (2.0 * q);
    double cosW  = std::cos(w0);

    double b0 =  1.0 + alpha * A;
    double b1 = -2.0 * cosW;
    double b2 =  1.0 - alpha * A;
    double a0 =  1.0 + alpha / A;
    double a1 = -2.0 * cosW;
    double a2 =  1.0 - alpha / A;

    f.b0 = b0 / a0;
    f.b1 = b1 / a0;
    f.b2 = b2 / a0;
    f.a1 = a1 / a0;
    f.a2 = a2 / a0;
}

void CabSim::CalcLowPass(Biquad& f, double sr, double freq, double q)
{
    double w0    = 2.0 * kPi * freq / sr;
    double cosW  = std::cos(w0);
    double sinW  = std::sin(w0);
    double alpha = sinW / (2.0 * q);

    double b0 = (1.0 - cosW) / 2.0;
    double b1 =  1.0 - cosW;
    double b2 = (1.0 - cosW) / 2.0;
    double a0 =  1.0 + alpha;
    double a1 = -2.0 * cosW;
    double a2 =  1.0 - alpha;

    f.b0 = b0 / a0;
    f.b1 = b1 / a0;
    f.b2 = b2 / a0;
    f.a1 = a1 / a0;
    f.a2 = a2 / a0;
}
