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
        const float c = mCorr.load();

        // Background
        g.FillRect(IColor{255, 30, 30, 30}, mRECT);

        // Active fill — spreads from center; higher correlation = narrower bar
        const float spread  = (1.0f - std::max(c, 0.0f)) * 0.5f;
        const float cx      = mRECT.MW();
        const float halfW   = mRECT.W() * spread;

        IColor barColor;
        if      (c > 0.7f) barColor = IColor{255,   0, 180,  80};  // green
        else if (c > 0.4f) barColor = IColor{255, 200, 160,   0};  // amber
        else               barColor = IColor{255, 204,  34,   0};  // red (accent)

        if (halfW > 0.5f)
        {
            IRECT fill { cx - halfW, mRECT.T, cx + halfW, mRECT.B };
            g.FillRect(barColor, fill);
        }

        // Center tick
        g.DrawLine(IColor{255, 255, 255, 255}, cx, mRECT.T, cx, mRECT.B, nullptr, 1.5f);

        // Border
        g.DrawRect(IColor{255, 80, 80, 80}, mRECT);
    }

private:
    std::atomic<float>& mCorr;
};

// WarningControl ---------------------------------------------------------------
// Shows a warning string when phase cancellation or hot input is detected.
// Hidden (draws nothing) when both conditions are false.
class WarningControl : public IControl
{
public:
    WarningControl(const IRECT& bounds, std::atomic<bool>& phaseWarn, const IText& text)
        : IControl(bounds), mPhaseWarn(phaseWarn), mText(text) { Hide(true); }

    void OnGUIIdle() override
    {
        const bool phase = mPhaseWarn.load();
        const bool clip  = GetDelegate()->GetParam(kGain)->Value() > 0.85;
        const bool show  = phase || clip;

        if (show)
        {
            mMsg = phase ? LS(kWarningPhaseCancel) : LS(kWarningClipHot);
            if (IsHidden()) { Hide(false); SetDirty(true); }
        }
        else
        {
            if (!IsHidden()) { Hide(true); }
        }
    }

    void Draw(IGraphics& g) override
    {
        if (mMsg)
            g.DrawText(mText, mMsg, mRECT);
    }

private:
    std::atomic<bool>& mPhaseWarn;
    IText              mText;
    const char*        mMsg = nullptr;
};
