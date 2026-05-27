#pragma once
#include <cassert>
#include <vector>

class IronWallPanner
{
public:
    void  OnReset(double sampleRate);
    void  ProcessBlock(float* mono, float* outL, float* outR, int nFrames);
    void  SetWidth(float width);
    float GetCorrelation() const;
    bool  GetWarning() const;

private:
    std::vector<float> mBufL;
    std::vector<float> mBufR;
    int   mBufSizeL  = 0;
    int   mBufSizeR  = 0;
    int   mWritePosL = 0;
    int   mWritePosR = 0;
    float mReadPosR  = 0.f;   // fractional read for R pitch shift
    int   mDelayL    = 0;
    int   mDelayR    = 0;
    float mWidth     = 1.f;
    float mCorrelation  = 1.f;
    bool  mWarning      = false;

    // Correlation accumulation
    float mSumLR        = 0.f;
    float mSumLL        = 0.f;
    float mSumRR        = 0.f;
    int   mCorrCounter  = 0;

    static constexpr float kPitchRatio       = 1.0011566336f; // std::pow(2, 2/1200)
    static constexpr int   kCorrelationWindow = 1024;
};
