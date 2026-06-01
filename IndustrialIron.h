#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "src/i18n/LocalizedStrings.h"
#include "src/dsp/AsymClipper.h"
#include "src/dsp/PreEQ.h"
#include "src/dsp/PostCompressor.h"
#include "src/dsp/IronWallPanner.h"
#include "src/dsp/CabSim.h"
#include <vector>

// Parameter indices — must match registration order in constructor
enum EParams
{
    kGain = 0,       // Input gain  0.0–1.0, maps to 0..+60 dB in DSP
    kTone,           // Pre-EQ tone balance 0.0–1.0
    kTightness,      // Post-compressor threshold 0.0–1.0
    kWidth,          // Iron Wall spread 0.0–1.0
    kCabOn,          // Cab sim toggle 0/1
    kParamOutput,    // Output trim 0.0–1.0 (prefixed to avoid clash with iplug::kOutput)
    kPreset,         // Preset selector 0/1/2
    kNumParams
};

// UI control tags
enum ECtrlTags
{
    kCtrlTagGainKnob = 0,
    kCtrlTagToneKnob,
    kCtrlTagTightnessKnob,
    kCtrlTagWidthKnob,
    kCtrlTagOutputKnob,
    kCtrlTagCabToggle,
    kCtrlTagPresetRadio,
    kCtrlTagTitle,
    kCtrlTagVersion,
    kNumCtrlTags
};

// Preset count per SPEC section 5
const int kNumPresets = 3;

using namespace iplug;
using namespace igraphics;

class IndustrialIron final : public Plugin
{
public:
    explicit IndustrialIron(const InstanceInfo& info);

#if IPLUG_EDITOR
    bool OnHostRequestingSupportedViewConfiguration(int width, int height) override { return true; }
    void OnParamChange(int paramIdx) override;
#endif

#if IPLUG_DSP
    void OnReset() override;
    void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
#endif

private:
    void LoadPreset(int presetIdx);

    // Preset data per SPEC section 5
    struct PresetData
    {
        float gain;
        float tone;
        float tightness;
        float width;
        float cabOn;
    };

    static constexpr PresetData kPresets[3] =
    {
        { 0.65f, 0.35f, 0.85f, 0.95f, 1.f },  // Rhythm Heavy
        { 0.65f, 0.65f, 0.50f, 0.60f, 1.f },  // Lead Cut
        { 0.30f, 0.50f, 0.20f, 0.00f, 0.f },  // DI Clean
    };

    int mCurrentPreset = -1;  // -1 = no preset active
    PreEQ           mPreEQ;
    AsymClipper     mAsymClipper;
    PostCompressor  mPostCompressor;
    CabSim          mCabSim;
    IronWallPanner  mIronWall;
    float           mCorrelation  = 0.f;
    bool            mPhaseWarning = false;

    // Conversion buffers for double→float→double around IronWallPanner
    // Pre-allocated in OnReset() to avoid heap allocation in ProcessBlock
    std::vector<float> mMonoF;
    std::vector<float> mOutLF;
    std::vector<float> mOutRF;
};
