#pragma once

#include "StringKeys.h"
#include "Strings_EN.h"
#include "Strings_ES.h"

namespace industrial_iron {

// Compile-time safety check: arrays must have exactly kCOUNT entries
static_assert(
    sizeof(kStrings_EN) / sizeof(kStrings_EN[0]) == static_cast<int>(StringKey::kCOUNT),
    "Strings_EN size mismatch — update the array to match StringKey::kCOUNT"
);
static_assert(
    sizeof(kStrings_ES) / sizeof(kStrings_ES[0]) == static_cast<int>(StringKey::kCOUNT),
    "Strings_ES size mismatch — update the array to match StringKey::kCOUNT"
);

enum class Locale { EN, ES };

class LocalizedStrings
{
public:
    static LocalizedStrings& Instance()
    {
        static LocalizedStrings instance;
        return instance;
    }

    void SetLocale(Locale locale) { mLocale = locale; }
    Locale GetLocale() const { return mLocale; }

    const char* Get(StringKey key) const
    {
        const int idx = static_cast<int>(key);
        switch (mLocale)
        {
            case Locale::ES: return kStrings_ES[idx];
            case Locale::EN: // fall-through
            default:         return kStrings_EN[idx];
        }
    }

private:
    LocalizedStrings() = default;
    Locale mLocale = Locale::EN;
};

} // namespace industrial_iron

// Convenience macro — keeps call sites short
#define LS(key) industrial_iron::LocalizedStrings::Instance().Get(industrial_iron::StringKey::key)
