#pragma once

namespace industrial_iron {

enum class StringKey : int
{
    // UI Labels
    kPluginName = 0,
    kGainLabel,
    kToneLabel,
    kTightnessLabel,
    kWidthLabel,
    kOutputLabel,
    kCabLabel,
    kPresetLabel,

    // Preset names
    kPresetRhythmHeavy,
    kPresetLeadCut,
    kPresetDIClean,

    // Warnings / Meters
    kWarningPhaseCancel,
    kWarningClipHot,
    kMeterCorrelation,

    kCOUNT
};

} // namespace industrial_iron
