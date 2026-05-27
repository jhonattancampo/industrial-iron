#pragma once
#include <cassert>
#include <cmath>

class CabSim
{
public:
    void OnReset(double sampleRate);
    void ProcessBlock(float* in, float* out, int nFrames);
    void SetEnabled(bool enabled);
    bool GetEnabled() const;

private:
    struct Biquad
    {
        double b0 = 1.0, b1 = 0.0, b2 = 0.0;
        double a1 = 0.0, a2 = 0.0;
        double x1 = 0.0, x2 = 0.0;
        double y1 = 0.0, y2 = 0.0;

        double Process(double x)
        {
            double y = b0*x + b1*x1 + b2*x2 - a1*y1 - a2*y2;
            x2 = x1; x1 = x;
            y2 = y1; y1 = y;
            return y;
        }

        void ClearState() { x1 = x2 = y1 = y2 = 0.0; }
    };

    void CalcLowShelf (Biquad& f, double sr, double freq, double gainDb);
    void CalcPeaking  (Biquad& f, double sr, double freq, double q, double gainDb);
    void CalcLowPass  (Biquad& f, double sr, double freq, double q);

    Biquad mLowShelf;   // Stage 1 — low-end body  (120 Hz, +3 dB)
    Biquad mBoxRes;     // Stage 2 — box resonance  (400 Hz, Q=2, +2 dB)
    Biquad mHighCut;    // Stage 3 — high cut       (5500 Hz, Q=0.7)
    Biquad mAirCut;     // Stage 4 — air cut        (8000 Hz, Q=1.0)

    double mSampleRate     = 44100.0;

    // Bypass / crossfade state
    bool   mTargetEnabled  = true;
    bool   mEnabled        = true;   // mirrors mTargetEnabled once fade completes
    int    mCrossfadePos   = 0;      // 0 = crossfade complete; >0 = samples remaining
    bool   mFadingIn       = true;   // true = ramping wet up, false = ramping wet down

    static constexpr int    kCrossfadeSamples = 64;
    static constexpr double kPi = 3.14159265358979323846;

    // Spec §4.4 constants
    static constexpr double kLowShelfFreq  = 120.0;
    static constexpr double kLowShelfGain  = 3.0;
    static constexpr double kBoxResFreq    = 400.0;
    static constexpr double kBoxResQ       = 2.0;
    static constexpr double kBoxResGain    = 2.0;
    static constexpr double kHighCutFreq   = 5500.0;
    static constexpr double kHighCutQ      = 0.7;
    static constexpr double kAirCutFreq    = 8000.0;
    static constexpr double kAirCutQ       = 1.0;
};
