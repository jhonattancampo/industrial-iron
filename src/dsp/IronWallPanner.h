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
    std::vector<float> mBuf;
    int   mBufSize      = 0;
    int   mWritePos     = 0;
    float mReadPos      = 0.f;
    int   mDelaySamples = 0;
    float mWidth        = 1.f;
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
