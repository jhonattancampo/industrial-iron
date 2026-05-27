#pragma once

#include "StringKeys.h"

namespace industrial_iron {

// Must match StringKey order exactly — verified by static_assert in LocalizedStrings.h
constexpr const char* kStrings_ES[static_cast<int>(StringKey::kCOUNT)] =
{
    // UI Labels
    "INDUSTRIAL IRON",        // kPluginName
    "GANANCIA",               // kGainLabel
    "TONO",                   // kToneLabel
    "AJUSTE",                 // kTightnessLabel
    "AMPLITUD",               // kWidthLabel
    "PAN",                    // kPanLabel
    "SALIDA",                 // kOutputLabel
    "CABINA",                 // kCabLabel
    "PRESET",                 // kPresetLabel

    // Preset names
    "Rhythm Heavy",           // kPresetRhythmHeavy
    "Lead Cut",               // kPresetLeadCut
    "DI Clean",               // kPresetDIClean

    // Warnings / Meters
    "Cancelacion de fase detectada - reduce la amplitud",  // kWarningPhaseCancel
    "Entrada muy alta - reduce la ganancia",               // kWarningClipHot
    "Correlacion",                                         // kMeterCorrelation
};

} // namespace industrial_iron
