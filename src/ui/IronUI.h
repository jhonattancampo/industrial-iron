#pragma once

// Included inside the #if IPLUG_EDITOR block of IndustrialIron.cpp.
// IndustrialIron.h (with EParams, LS macro, namespaces) is already included above.

#include "IControl.h"
#include "src/i18n/LocalizedStrings.h"
#include <atomic>
#include <cmath>

// CorrelationMeterControl -------------------------------------------------------
// A horizontal meter (~300 px wide, ~12 px tall) displaying L/R phase correlation.
// Reads from std::atomic<float>& — safe for audio-thread writes, UI-thread reads.
//
// NOTE: OnGUIIdle() requires USE_IDLE_CALLS which is NOT defined in this project.
// Per-frame updates are driven by SetAnimation() instead: IGraphics::IsDirty()
// calls ForAllControls(&IControl::Animate) unconditionally every frame, and
// IControl::IsDirty() returns true whenever an animation function is set —
// so the control redraws every frame without any preprocessor flag.
class CorrelationMeterControl : public IControl
{
public:
    CorrelationMeterControl(const IRECT& bounds, std::atomic<float>& corr)
        : IControl(bounds), mCorr(corr)
    {
        // Register for unconditional per-frame redraws via the animation pass.
        // An empty (but non-null) animation function makes IsDirty() always true.
        SetAnimation([](IControl*) {});
    }

    void Draw(IGraphics& g) override
    {
        const float c    = mCorr.load();
        const float absc = std::fabs(c);

        // Background: solid color based on correlation quality
        IColor bgColor;
        if      (absc > 0.8f) bgColor = IColor{255,   0, 204,  68};  // green  #00CC44
        else if (absc > 0.5f) bgColor = IColor{255, 204, 170,   0};  // yellow #CCAA00
        else                  bgColor = IColor{255, 204,  34,   0};  // red    #CC2200

        g.FillRect(bgColor, mRECT);

        // Moving white indicator line — maps corr [-1..1] to full width
        const float t = (c + 1.0f) / 2.0f;  // 0..1
        const float x = mRECT.L + t * mRECT.W();
        g.FillRect(IColor{255, 255, 255, 255},
                   IRECT{x - 1.5f, mRECT.T, x + 1.5f, mRECT.B});

        // Border
        g.DrawRect(IColor{255, 60, 60, 60}, mRECT);
    }

private:
    std::atomic<float>& mCorr;
};

// WarningControl ---------------------------------------------------------------
// Shows a warning string when phase cancellation or hot input is detected.
// Renders nothing when no warning is active (control area is transparent).
// Same SetAnimation() pattern as CorrelationMeterControl for per-frame updates.
class WarningControl : public IControl
{
public:
    WarningControl(const IRECT& bounds, std::atomic<bool>& phaseWarn, const IText& text)
        : IControl(bounds), mPhaseWarn(phaseWarn), mText(text)
    {
        SetAnimation([](IControl*) {});
    }

    void Draw(IGraphics& g) override
    {
        if (!GetDelegate()) return;

        const bool phase = mPhaseWarn.load();
        const bool clip  = GetDelegate()->GetParam(kGain)->Value() > 0.85;

        if (!phase && !clip) return;  // render nothing — transparent when inactive

        const char* msg = phase ? LS(kWarningPhaseCancel) : LS(kWarningClipHot);
        g.DrawText(mText, msg, mRECT);
    }

private:
    std::atomic<bool>& mPhaseWarn;
    IText              mText;
};
