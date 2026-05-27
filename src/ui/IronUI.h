#pragma once

// Included inside the #if IPLUG_EDITOR block of IndustrialIron.cpp.
// IndustrialIron.h (with EParams, LS macro, namespaces) is already included above.

#include "IControl.h"
#include "src/i18n/LocalizedStrings.h"
#include <atomic>
#include <algorithm>
#include <cmath>

// CorrelationMeterControl -------------------------------------------------------
// A horizontal meter (~300 px wide, ~12 px tall) displaying L/R phase correlation.
// Reads from std::atomic<float>& — safe for audio-thread writes, UI-thread reads.
class CorrelationMeterControl : public IControl
{
public:
    CorrelationMeterControl(const IRECT& bounds, std::atomic<float>& corr)
        : IControl(bounds), mCorr(corr) {}

    void OnGUIIdle() override { SetDirty(true); }

    void Draw(IGraphics& g) override
    {
        const float c    = mCorr.load();
        const float absc = std::fabs(c);

        // Background: solid color based on correlation quality
        IColor bgColor;
        if      (absc > 0.8f) bgColor = IColor{255,   0, 204,  68};  // green
        else if (absc > 0.5f) bgColor = IColor{255, 204, 170,   0};  // yellow
        else                  bgColor = IColor{255, 204,  34,   0};  // red

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
// Always attached and visible to the graphics engine; renders nothing when
// no warning is active. (iPlug2 does not call OnGUIIdle on hidden controls
// in some hosting configurations, so we keep the control always visible and
// draw nothing instead of hiding.)
class WarningControl : public IControl
{
public:
    WarningControl(const IRECT& bounds, std::atomic<bool>& phaseWarn, const IText& text)
        : IControl(bounds), mPhaseWarn(phaseWarn), mText(text) {}

    void OnGUIIdle() override
    {
        const bool phase = mPhaseWarn.load();
        const bool clip  = GetDelegate() && GetDelegate()->GetParam(kGain)->Value() > 0.85;
        const bool show  = phase || clip;

        if (show != mShowing)
        {
            mShowing = show;
            if (show)
                mMsg = phase ? LS(kWarningPhaseCancel) : LS(kWarningClipHot);
            SetDirty(true);
        }
    }

    void Draw(IGraphics& g) override
    {
        if (!mShowing || !mMsg) return;  // transparent when inactive
        g.DrawText(mText, mMsg, mRECT);
    }

private:
    std::atomic<bool>& mPhaseWarn;
    IText              mText;
    const char*        mMsg     = nullptr;
    bool               mShowing = false;
};
