#pragma once

#include "StringKeys.h"

namespace industrial_iron {

// Must match StringKey order exactly — verified by static_assert in LocalizedStrings.h
constexpr const char* kStrings_EN[static_cast<int>(StringKey::kCOUNT)] =
{
    // UI Labels
    "INDUSTRIAL IRON",       // kPluginName
    "GAIN",                  // kGainLabel
    "TONE",                  // kToneLabel
    "TIGHTNESS",             // kTightnessLabel
    "WIDTH",                 // kWidthLabel
    "OUTPUT",                // kOutputLabel
    "CAB",                   // kCabLabel
    "PRESET",                // kPresetLabel

    // Preset names
    "Rhythm Heavy",          // kPresetRhythmHeavy
    "Lead Cut",              // kPresetLeadCut
    "DI Clean",              // kPresetDIClean

    // Warnings / Meters
    "Phase cancellation detected - reduce Width",  // kWarningPhaseCancel
    "Input too hot - reduce Gain",                 // kWarningClipHot
    "Correlation",                                 // kMeterCorrelation
};

} // namespace industrial_iron
