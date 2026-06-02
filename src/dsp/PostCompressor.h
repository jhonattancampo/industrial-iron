#pragma once
#include <cassert>
#include <cmath>

class PostCompressor
{
public:
    void   OnReset(double sampleRate);
    void   SetTightness(float t);
    double Process(double sample);
    void   Reset();

private:
    double mSampleRate   = 44100.0;
    double mAttackCoeff  = 0.0;
    double mReleaseCoeff = 0.0;
    double mEnvelope     = 0.0;   // RMS envelope state (squared domain)
    double mThresholdDb  = -20.0;
    double mMakeupGain   = 1.0;

    static constexpr double kRatio       = 4.0;
    static constexpr double kAttackMs    = 5.0;
    static constexpr double kReleaseMs   = 80.0;

    void RecalcMakeupGain();
};
