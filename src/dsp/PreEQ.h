#pragma once
#include <cassert>
#include <cmath>

class PreEQ
{
public:
    void   OnReset(double sampleRate);
    void   SetTone(float t);
    double Process(double sample);
    void   Reset();

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

    void CalcHighPass(Biquad& f, double sr, double freq, double q);
    void CalcPeaking(Biquad& f, double sr, double freq, double q, double gainDb);

    Biquad mHP;        // 80 Hz high-pass
    Biquad mLowMid;    // 280 Hz peak  (kTone-modulated)
    Biquad mMidScoop;  // 1000 Hz peak (fixed -4 dB)
    Biquad mPresence;  // 3500 Hz peak (kTone-modulated inverse)

    double mSampleRate = 44100.0;
    float  mTone       = 0.5f;

    static constexpr double kPi = 3.14159265358979323846;

    // Spec §4.1 constants
    static constexpr double kHPFreq        = 80.0;
    static constexpr double kHPQ           = 0.7;
    static constexpr double kLowMidFreq    = 280.0;
    static constexpr double kLowMidQ       = 1.2;
    static constexpr double kMidScoopFreq  = 1000.0;
    static constexpr double kMidScoopQ     = 1.5;
    static constexpr double kMidScoopGain  = -7.0;
    static constexpr double kPresenceFreq  = 3500.0;
    static constexpr double kPresenceQ     = 1.0;
};
