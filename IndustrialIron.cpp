#include "IndustrialIron.h"
#include "IPlug_include_in_plug_src.h"
#include <cmath>

#if IPLUG_EDITOR
#include "IControls.h"
#endif

// ---- Colours per SPEC section 7.2 ----------------------------------------
static const IColor kColorBackground  { 255,  10,  10,  10 }; // #0A0A0A
static const IColor kColorAccent      { 255, 204,  34,   0 }; // #CC2200
static const IColor kColorKnobTrack   { 255,  50,  50,  50 };
static const IColor kColorText        { 255, 220, 220, 220 };
static const IColor kColorPresetOn    { 255, 204,  34,   0 }; // accent when selected
static const IColor kColorPresetOff   { 255,  45,  45,  45 };

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

    // Register presets per SPEC section 5
    MakePreset(LS(kPresetRhythmHeavy),
        kPresets[0].gain, kPresets[0].tone, kPresets[0].tightness,
        kPresets[0].width, kPresets[0].cabOn, 0.5f, 0.f);

    MakePreset(LS(kPresetLeadCut),
        kPresets[1].gain, kPresets[1].tone, kPresets[1].tightness,
        kPresets[1].width, kPresets[1].cabOn, 0.5f, 1.f);

    MakePreset(LS(kPresetDIClean),
        kPresets[2].gain, kPresets[2].tone, kPresets[2].tightness,
        kPresets[2].width, kPresets[2].cabOn, 0.5f, 2.f);

#if IPLUG_EDITOR
    mMakeGraphicsFunc = [&]() {
        return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS);
    };

    mLayoutFunc = [&](IGraphics* pGraphics) {
        const IRECT bounds = pGraphics->GetBounds();

        // Resize: just reposition existing controls
        if (pGraphics->NControls()) {
            pGraphics->GetBackgroundControl()->SetTargetAndDrawRECTs(bounds);
            return;
        }

        // ---- Setup ----
        pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
        pGraphics->AttachPanelBackground(kColorBackground);

        // ---- Typography ----
        const IText titleText  { 18.f, kColorAccent,    "Roboto-Regular", EAlign::Near };
        const IText versionText{ 11.f, kColorText,      "Roboto-Regular", EAlign::Far  };
        const IText labelText  { 11.f, kColorText,      "Roboto-Regular", EAlign::Center };
        const IText presetText { 12.f, kColorText,      "Roboto-Regular", EAlign::Center };

        // ---- Header row (top 36 px) ----
        const IRECT header     = bounds.GetFromTop(36.f);
        const IRECT titleRect  = header.GetFromLeft(260.f).GetPadded(-8.f);
        const IRECT versionRect= header.GetFromRight(80.f).GetPadded(-8.f);
        const IRECT presetArea = header.GetMidHPadded(180.f);  // centred preset buttons

        // ---- Knob row (rows 36–220, centered) ----
        const IRECT knobRow  = bounds.GetReducedFromTop(36.f).GetReducedFromBottom(80.f);
        const float kKnobW   = bounds.W() / 5.f;
        const float kKnobH   = knobRow.H();

        const IRECT gainRect  = knobRow.GetGridCell(0, 0, 1, 5).GetPadded(-8.f);
        const IRECT toneRect  = knobRow.GetGridCell(0, 1, 1, 5).GetPadded(-8.f);
        const IRECT tightRect = knobRow.GetGridCell(0, 2, 1, 5).GetPadded(-8.f);
        const IRECT widthRect = knobRow.GetGridCell(0, 3, 1, 5).GetPadded(-8.f);
        const IRECT outputRect= knobRow.GetGridCell(0, 4, 1, 5).GetPadded(-8.f);

        // ---- Bottom row (last 80 px) ----
        const IRECT bottomRow = bounds.GetFromBottom(80.f).GetPadded(-10.f);
        const IRECT cabRect   = bottomRow.GetFromLeft(140.f);

        // ---- IVStyle for knobs ----
        const IVStyle knobStyle = IVStyle()
            .WithColor(kBG,     kColorBackground)
            .WithColor(kFG,     kColorKnobTrack)
            .WithColor(kX1,     kColorAccent)
            .WithColor(kHL,     kColorAccent)
            .WithColor(kON,     kColorAccent)
            .WithLabelText(labelText)
            .WithValueText(labelText.WithSize(9.f));

        const IVStyle toggleStyle = IVStyle()
            .WithColor(kBG,   kColorBackground)
            .WithColor(kFG,   kColorKnobTrack)
            .WithColor(kX1,   kColorAccent)
            .WithColor(kON,   kColorAccent)
            .WithLabelText(labelText);

        const IVStyle presetStyle = IVStyle()
            .WithColor(kBG,  kColorPresetOff)
            .WithColor(kFG,  kColorPresetOff)
            .WithColor(kX1,  kColorPresetOn)
            .WithColor(kON,  kColorPresetOn)
            .WithColor(kHL,  kColorAccent)
            .WithLabelText(presetText);

        // ---- Title & version ----
        pGraphics->AttachControl(
            new ITextControl(titleRect, LS(kPluginName), titleText),
            kCtrlTagTitle);
        WDL_String buildStr;
        GetBuildInfoStr(buildStr, __DATE__, __TIME__);
        pGraphics->AttachControl(
            new ITextControl(versionRect, buildStr.Get(), versionText),
            kCtrlTagVersion);

        // ---- 5 Knobs ----
        pGraphics->AttachControl(
            new IVKnobControl(gainRect,   kGain,      LS(kGainLabel),      knobStyle), kCtrlTagGainKnob);
        pGraphics->AttachControl(
            new IVKnobControl(toneRect,   kTone,      LS(kToneLabel),      knobStyle), kCtrlTagToneKnob);
        pGraphics->AttachControl(
            new IVKnobControl(tightRect,  kTightness, LS(kTightnessLabel), knobStyle), kCtrlTagTightnessKnob);
        pGraphics->AttachControl(
            new IVKnobControl(widthRect,  kWidth,     LS(kWidthLabel),     knobStyle), kCtrlTagWidthKnob);
        pGraphics->AttachControl(
            new IVKnobControl(outputRect, kParamOutput, LS(kOutputLabel),  knobStyle), kCtrlTagOutputKnob);

        // ---- CAB toggle ----
        pGraphics->AttachControl(
            new IVToggleControl(cabRect, kCabOn, LS(kCabLabel), toggleStyle),
            kCtrlTagCabToggle);

        // ---- Preset selector (3 buttons acting as a radio group) ----
        const float presetBtnW = presetArea.W() / 3.f;
        const IRECT p0 = presetArea.GetGridCell(0, 0, 1, 3).GetPadded(-3.f);
        const IRECT p1 = presetArea.GetGridCell(0, 1, 1, 3).GetPadded(-3.f);
        const IRECT p2 = presetArea.GetGridCell(0, 2, 1, 3).GetPadded(-3.f);

        pGraphics->AttachControl(
            new IVButtonControl(p0, [&](IControl* pCtrl) {
                GetParam(kPreset)->Set(0.);
                InformHostOfParamChange(kPreset, 0.);
                LoadPreset(0);
                pCtrl->GetDelegate()->SendParameterValueFromUI(kPreset, 0.);
            }, LS(kPresetRhythmHeavy), presetStyle), kCtrlTagPreset0);

        pGraphics->AttachControl(
            new IVButtonControl(p1, [&](IControl* pCtrl) {
                GetParam(kPreset)->Set(0.5);
                InformHostOfParamChange(kPreset, 0.5);
                LoadPreset(1);
                pCtrl->GetDelegate()->SendParameterValueFromUI(kPreset, 0.5);
            }, LS(kPresetLeadCut), presetStyle), kCtrlTagPreset1);

        pGraphics->AttachControl(
            new IVButtonControl(p2, [&](IControl* pCtrl) {
                GetParam(kPreset)->Set(1.);
                InformHostOfParamChange(kPreset, 1.);
                LoadPreset(2);
                pCtrl->GetDelegate()->SendParameterValueFromUI(kPreset, 1.);
            }, LS(kPresetDIClean), presetStyle), kCtrlTagPreset2);
    };
#endif
}

// ---- Preset loader ---------------------------------------------------------
void IndustrialIron::LoadPreset(int presetIdx)
{
    if (presetIdx < 0 || presetIdx > 2) return;
    mCurrentPreset = presetIdx;
    const PresetData& p = kPresets[presetIdx];
    GetParam(kGain)->Set(p.gain);
    GetParam(kTone)->Set(p.tone);
    GetParam(kTightness)->Set(p.tightness);
    GetParam(kWidth)->Set(p.width);
    GetParam(kCabOn)->Set(p.cabOn);
    InformHostOfParamChange(kGain,      p.gain);
    InformHostOfParamChange(kTone,      p.tone);
    InformHostOfParamChange(kTightness, p.tightness);
    InformHostOfParamChange(kWidth,     p.width);
    InformHostOfParamChange(kCabOn,     p.cabOn);
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
    // Phase 3 chain: InputGain → PreEQ → AsymClipper → PostCompressor → IronWallPanner → OutputGain

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

    // OutputGain + copy back to double outputs
    for (int s = 0; s < nFrames; ++s)
    {
        outputs[0][s] = static_cast<sample>(mOutLF[s] * outLin);
        outputs[1][s] = static_cast<sample>(mOutRF[s] * outLin);
    }

    // Store metering state for UI polling (Phase 5)
    mCorrelation  = mIronWall.GetCorrelation();
    mPhaseWarning = mIronWall.GetWarning();
}
#endif
