#include "IndustrialIron.h"
#include "IPlug_include_in_plug_src.h"
#include <cmath>

#if IPLUG_EDITOR && !defined(NO_IGRAPHICS)
#include "IControls.h"
#endif

#ifdef WEBVIEW_EDITOR_DELEGATE
#include "IPlugPaths.h"
#endif

#ifndef NO_IGRAPHICS
// ---- Colours per SPEC section 7.2 (IGraphics builds only) ------------------
static const IColor kColorBackground  { 255,  10,  10,  10 }; // #0A0A0A
static const IColor kColorAccent      { 255, 204,  34,   0 }; // #CC2200
static const IColor kColorKnobTrack   { 255,  50,  50,  50 };
static const IColor kColorText        { 255, 220, 220, 220 };
static const IColor kColorPresetOn    { 255, 204,  34,   0 }; // accent when selected
static const IColor kColorPresetOff   { 255,  45,  45,  45 };
#endif

// ---- Constructor -----------------------------------------------------------
IndustrialIron::IndustrialIron(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
    // Register parameters per SPEC section 3
    GetParam(kGain)->InitDouble(LS(kGainLabel),       0.7, 0.0, 1.0, 0.001);
    GetParam(kTone)->InitDouble(LS(kToneLabel),       0.5, 0.0, 1.0, 0.001);
    GetParam(kTightness)->InitDouble(LS(kTightnessLabel), 0.6, 0.0, 1.0, 0.001);
    GetParam(kWidth)->InitDouble(LS(kWidthLabel),     0.8, 0.0, 1.0, 0.001);
    GetParam(kCabOn)->InitBool(LS(kCabLabel),         true);
    GetParam(kParamOutput)->InitDouble(LS(kOutputLabel),   0.5, 0.0, 1.0, 0.001);
    GetParam(kPreset)->InitInt(LS(kPresetLabel),      0,   0,   2);
    GetParam(kPan)->InitDouble(LS(kPanLabel),          0.0, -1.0, 1.0, 0.001);

    // Register presets per SPEC section 5
    MakePreset(LS(kPresetRhythmHeavy),
        kPresets[0].gain, kPresets[0].tone, kPresets[0].tightness,
        kPresets[0].width, kPresets[0].cabOn, 0.5f, 0.f, kPresets[0].pan);

    MakePreset(LS(kPresetLeadCut),
        kPresets[1].gain, kPresets[1].tone, kPresets[1].tightness,
        kPresets[1].width, kPresets[1].cabOn, 0.5f, 1.f, kPresets[1].pan);

    MakePreset(LS(kPresetDIClean),
        kPresets[2].gain, kPresets[2].tone, kPresets[2].tightness,
        kPresets[2].width, kPresets[2].cabOn, 0.5f, 2.f, kPresets[2].pan);

#ifdef WEBVIEW_EDITOR_DELEGATE
    mEditorInitFunc = [&]() {
        LoadIndexHtml(__FILE__, GetBundleID());
        EnableScroll(false);
    };
#ifdef DEBUG
    SetEnableDevTools(true);
#endif
#endif
}

// ---- Preset loader ---------------------------------------------------------
void IndustrialIron::LoadPreset(int presetIdx)
{
    if (presetIdx < 0 || presetIdx > 2) return;
    mCurrentPreset = presetIdx;
    const PresetData& p = kPresets[presetIdx];

    // SetNormalized sets the stored value. SendCurrentParamValuesFromDelegate
    // then calls SendParameterValueFromDelegate for every param, which calls
    // SetValueFromDelegate on each linked UI control — this is what makes
    // knobs visually update. SendParameterValueFromUI goes the wrong direction
    // (UI→Delegate) and does not update controls. This was the root cause bug.
    GetParam(kGain)->SetNormalized(p.gain);
    GetParam(kTone)->SetNormalized(p.tone);
    GetParam(kTightness)->SetNormalized(p.tightness);
    GetParam(kWidth)->SetNormalized(p.width);
    GetParam(kCabOn)->SetNormalized(p.cabOn);
    GetParam(kPreset)->Set(static_cast<double>(presetIdx));
    GetParam(kPan)->Set(static_cast<double>(p.pan));  // actual value: -1..+1
    SendCurrentParamValuesFromDelegate();

    auto notifyHost = [&](int idx, double normVal) {
        BeginInformHostOfParamChange(idx);
        InformHostOfParamChange(idx, normVal);
        EndInformHostOfParamChange(idx);
    };
    notifyHost(kGain,       p.gain);
    notifyHost(kTone,       p.tone);
    notifyHost(kTightness,  p.tightness);
    notifyHost(kWidth,      p.width);
    notifyHost(kCabOn,      p.cabOn);
    notifyHost(kPan,        (p.pan + 1.0) / 2.0);  // normalize -1..+1 → 0..1

#if IPLUG_EDITOR && !defined(NO_IGRAPHICS)
    if (auto* pUI = GetUI())
    {
        if (auto* pCtrl = pUI->GetControlWithTag(kCtrlTagPresetRadio))
        {
            pCtrl->SetValue(static_cast<double>(presetIdx) / 2.0);
            pCtrl->SetDirty(false);
        }
    }
#endif
}

#if IPLUG_EDITOR
// ---- OnParamChange ---------------------------------------------------------
void IndustrialIron::OnParamChange(int paramIdx)
{
    if (paramIdx == kPreset) {
        const int idx = static_cast<int>(GetParam(kPreset)->Value() + 0.5);
        LoadPreset(idx);
    }
}

// ---- OnIdle ----------------------------------------------------------------
// Called by the host on the UI thread each display frame (~60 Hz).
// Pushes the current correlation float to the WebView so the JS meter
// updates reactively. Throttled to ~30 Hz to keep the bridge cheap.
void IndustrialIron::OnIdle()
{
#ifdef WEBVIEW_EDITOR_DELEGATE
    if (++mIdleFrameCount >= 2)
    {
        mIdleFrameCount = 0;
        float corr = mIronWall.GetCorrelation();
        SendArbitraryMsgFromDelegate(kMsgTagCorrelation, sizeof(float), &corr);
    }
#endif
}
#endif

// ---- DSP -------------------------------------------------------------------
#if IPLUG_DSP
void IndustrialIron::OnReset()
{
    const double sr = GetSampleRate();
    mPreEQ.OnReset(sr);
    mAsymClipper.OnReset(sr);
    mPostCompressor.OnReset(sr);
    mCabSim.OnReset(sr);
    mIronWall.OnReset(sr);
    SetLatency(mAsymClipper.GetLatency());

    const int blockSize = GetBlockSize();
    mMonoF.resize(blockSize);
    mOutLF.resize(blockSize);
    mOutRF.resize(blockSize);
}

void IndustrialIron::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
    // Phase 4 chain: InputGain → PreEQ → AsymClipper → PostCompressor → CabSim → IronWallPanner → OutputGain

    // kGain: 0–1 → -20 to +40 dB
    const double gainDb  = -20.0 + 60.0 * GetParam(kGain)->Value();
    const double gainLin = std::pow(10.0, gainDb / 20.0);

    // kParamOutput: 0–1 → -20 to +6 dB
    const double outDb   = -20.0 + 26.0 * GetParam(kParamOutput)->Value();
    const double outLin  = std::pow(10.0, outDb  / 20.0);

    // Update tone/tightness/cabOn once per block (coefficient update is cheap, no audio artifacts)
    mPreEQ.SetTone(static_cast<float>(GetParam(kTone)->Value()));
    mPostCompressor.SetTightness(static_cast<float>(GetParam(kTightness)->Value()));
    mCabSim.SetEnabled(GetParam(kCabOn)->Bool());

    // InputGain + PreEQ → outputs[0] (AsymClipper reads from here)
    for (int s = 0; s < nFrames; ++s)
        outputs[0][s] = static_cast<sample>(mPreEQ.Process(inputs[0][s] * gainLin));

    // Hard clip with x4 oversampling (in-place)
    mAsymClipper.ProcessBlock(outputs[0], outputs[0], nFrames);

    // PostCompressor → float conversion (IronWallPanner uses float*, sample is double)
    for (int s = 0; s < nFrames; ++s)
        mMonoF[s] = static_cast<float>(mPostCompressor.Process(outputs[0][s]));

    // CabSim: mono → mono (in-place)
    mCabSim.ProcessBlock(mMonoF.data(), mMonoF.data(), nFrames);

    // IronWallPanner: mono → stereo
    const float width = static_cast<float>(GetParam(kWidth)->Value());
    mIronWall.SetWidth(width);
    mIronWall.ProcessBlock(mMonoF.data(), mOutLF.data(), mOutRF.data(), nFrames);

    // Equal-power pan (kPan: -1=full L, 0=center, +1=full R)
    const double panAngle = GetParam(kPan)->Value() * (M_PI / 4.0);
    const double gainL    = std::cos(panAngle + M_PI / 4.0);
    const double gainR    = std::cos(panAngle - M_PI / 4.0);

    // OutputGain + pan + copy back to double outputs
    for (int s = 0; s < nFrames; ++s)
    {
        outputs[0][s] = static_cast<sample>(mOutLF[s] * outLin * gainL);
        outputs[1][s] = static_cast<sample>(mOutRF[s] * outLin * gainR);
    }

    // Store metering state for UI polling (Phase 5)
    mCorrelation  = mIronWall.GetCorrelation();
    mPhaseWarning = mIronWall.GetWarning();
}
#endif
