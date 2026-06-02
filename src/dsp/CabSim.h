#pragma once
#include <array>
#include <cmath>
#include "ir/CabIR_Marshall.h"
#include "ir/CabIR_Mesa.h"
#include "ir/CabIR_V30.h"

class CabSim
{
public:
    void OnReset(double sampleRate);
    void ProcessBlock(float* in, float* out, int nFrames);
    void SetEnabled(bool enabled);
    bool GetEnabled() const;
    void SetCabType(int type);  // 0=Marshall, 1=Mesa, 2=V30

private:
    static constexpr int kIRLen            = 128;
    static constexpr int kMask             = kIRLen - 1;
    static constexpr int kCrossfadeSamples = 64;

    // High-shelf post-cab — rolls off digital harshness above 7kHz
    static constexpr float kHSFreq = 7000.f;
    static constexpr float kHSGain = -3.f;   // dB
    static constexpr float kHSQ    = 0.7f;
    static constexpr float kPi     = 3.14159265358979f;

    struct Biquad
    {
        float b0 = 1.f, b1 = 0.f, b2 = 0.f;
        float a1 = 0.f, a2 = 0.f;
        float x1 = 0.f, x2 = 0.f;
        float y1 = 0.f, y2 = 0.f;

        float Process(float x)
        {
            float y = b0*x + b1*x1 + b2*x2 - a1*y1 - a2*y2;
            x2 = x1; x1 = x;
            y2 = y1; y1 = y;
            return y;
        }
        void Clear() { x1 = x2 = y1 = y2 = 0.f; }
    };

    void CalcHighShelf(Biquad& f, float sr, float freq, float gainDb, float q);

    std::array<float, kIRLen> mBuf = {};
    int          mWritePos    = 0;
    const float* mActiveIR    = kIR_Marshall;
    Biquad       mHighShelf;

    bool  mTargetEnabled  = true;
    bool  mEnabled        = true;
    int   mCrossfadePos   = 0;
    bool  mFadingIn       = true;
};
