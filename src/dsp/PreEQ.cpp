#include <cassert>
#include "PreEQ.h"

void PreEQ::OnReset(double sampleRate)
{
    assert(sampleRate > 0.0);
    mSampleRate = sampleRate;

    CalcHighPass(mHP, sampleRate, kHPFreq, kHPQ);
    CalcPeaking(mMidScoop, sampleRate, kMidScoopFreq, kMidScoopQ, kMidScoopGain);

    // Modulated filters use current mTone value
    float t = mTone;
    double lowMidGain   = 8.0 + (2.0 - 8.0) * static_cast<double>(t);
    double presenceGain = 1.0 + (5.0 - 1.0) * static_cast<double>(t);
    CalcPeaking(mLowMid,   sampleRate, kLowMidFreq,   kLowMidQ,   lowMidGain);
    CalcPeaking(mPresence, sampleRate, kPresenceFreq, kPresenceQ, presenceGain);
}

void PreEQ::SetTone(float t)
{
    mTone = t;
    double lowMidGain   = 8.0 + (2.0 - 8.0) * static_cast<double>(t);
    double presenceGain = 1.0 + (5.0 - 1.0) * static_cast<double>(t);
    CalcPeaking(mLowMid,   mSampleRate, kLowMidFreq,   kLowMidQ,   lowMidGain);
    CalcPeaking(mPresence, mSampleRate, kPresenceFreq, kPresenceQ, presenceGain);
}

double PreEQ::Process(double sample)
{
    sample = mHP.Process(sample);
    sample = mLowMid.Process(sample);
    sample = mMidScoop.Process(sample);
    sample = mPresence.Process(sample);
    return sample;
}

void PreEQ::Reset()
{
    mHP.ClearState();
    mLowMid.ClearState();
    mMidScoop.ClearState();
    mPresence.ClearState();
}

void PreEQ::CalcHighPass(Biquad& f, double sr, double freq, double q)
{
    double w0    = 2.0 * kPi * freq / sr;
    double alpha = std::sin(w0) / (2.0 * q);
    double cosW  = std::cos(w0);

    double b0 =  (1.0 + cosW) / 2.0;
    double b1 = -(1.0 + cosW);
    double b2 =  (1.0 + cosW) / 2.0;
    double a0 =   1.0 + alpha;
    double a1 =  -2.0 * cosW;
    double a2 =   1.0 - alpha;

    f.b0 = b0 / a0;
    f.b1 = b1 / a0;
    f.b2 = b2 / a0;
    f.a1 = a1 / a0;
    f.a2 = a2 / a0;
}

void PreEQ::CalcPeaking(Biquad& f, double sr, double freq, double q, double gainDb)
{
    double w0    = 2.0 * kPi * freq / sr;
    double A     = std::pow(10.0, gainDb / 40.0);
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
