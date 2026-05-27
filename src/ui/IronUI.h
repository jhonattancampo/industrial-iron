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
        g.FillRect(IColor{255, 20, 20, 20}, mRECT);

        const float cx     = mRECT.MW();
        const float spread = (1.0f - std::max(c, 0.0f)) * 0.5f;  // 0..0.5
        const float halfW  = mRECT.W() * spread;

        if (halfW > 0.5f)
        {
            // Zone boundaries as fraction of half-width of full bar
            const float halfFull = mRECT.W() * 0.5f;
            const float z1 = halfFull * 0.25f;  // green fills inner 25%
            const float z2 = halfFull * 0.55f;  // yellow fills up to 55%
            // red fills beyond 55% (to halfW)

            // Draw from outside in so inner zones paint over outer
            // Red zone (outermost)
            g.FillRect(IColor{255, 204, 34, 0},
                IRECT{cx - halfW, mRECT.T, cx + halfW, mRECT.B});

            // Yellow zone (overwrites inner portion of red)
            if (halfW > 0.f)
                g.FillRect(IColor{255, 204, 170, 0},
                    IRECT{cx - std::min(halfW, z2), mRECT.T, cx + std::min(halfW, z2), mRECT.B});

            // Green zone (overwrites inner portion of yellow)
            if (halfW > 0.f)
                g.FillRect(IColor{255, 0, 204, 68},
                    IRECT{cx - std::min(halfW, z1), mRECT.T, cx + std::min(halfW, z1), mRECT.B});
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
