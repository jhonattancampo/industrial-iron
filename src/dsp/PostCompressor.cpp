#include <cassert>
#include "PostCompressor.h"

void PostCompressor::OnReset(double sampleRate)
{
    assert(sampleRate > 0.0);
    mSampleRate   = sampleRate;
    mAttackCoeff  = std::exp(-1.0 / (sampleRate * kAttackMs  * 0.001));
    mReleaseCoeff = std::exp(-1.0 / (sampleRate * kReleaseMs * 0.001));
    mEnvelope     = 0.0;
}

void PostCompressor::SetTightness(float t)
{
    // kTightness 0.0 → -40 dBFS, 1.0 → -6 dBFS
    mThresholdDb = -40.0 + (-6.0 - (-40.0)) * static_cast<double>(t);
    RecalcMakeupGain();
}

double PostCompressor::Process(double sample)
{
    // Feed-forward RMS detection (squared domain)
    double sq = sample * sample;
    if (sq > mEnvelope)
        mEnvelope = mAttackCoeff  * mEnvelope + (1.0 - mAttackCoeff)  * sq;
    else
        mEnvelope = mReleaseCoeff * mEnvelope + (1.0 - mReleaseCoeff) * sq;

    double rmsLinear = std::sqrt(mEnvelope + 1e-30);
    double rmsDb     = 20.0 * std::log10(rmsLinear + 1e-30);

    double gainReductionDb = 0.0;
    if (rmsDb > mThresholdDb)
        gainReductionDb = (mThresholdDb - rmsDb) * (1.0 - 1.0 / kRatio);

    double gainLinear = std::pow(10.0, gainReductionDb / 20.0);
    return sample * gainLinear * mMakeupGain;
}

void PostCompressor::Reset()
{
    mEnvelope = 0.0;
}

void PostCompressor::RecalcMakeupGain()
{
    // Unity-gain tracking: compensate for gain reduction applied at threshold level
    // gainReductionAtThreshold = thresholdDb * (1 - 1/ratio) — negate to get makeup
    mMakeupGain = std::pow(10.0, -mThresholdDb * (1.0 - 1.0 / kRatio) / 20.0);
}
