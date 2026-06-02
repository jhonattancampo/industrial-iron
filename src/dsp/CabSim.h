#pragma once
#include <array>
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
    static constexpr int kIRLen             = 128;
    static constexpr int kMask              = kIRLen - 1;
    static constexpr int kCrossfadeSamples  = 64;

    std::array<float, kIRLen> mBuf = {};
    int          mWritePos    = 0;
    const float* mActiveIR    = kIR_Marshall;

    bool  mTargetEnabled  = true;
    bool  mEnabled        = true;
    int   mCrossfadePos   = 0;
    bool  mFadingIn       = true;
};
