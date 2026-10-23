#pragma once
#include <algorithm>
#include <cstdint>

namespace handhud {
inline bool HideHealth(bool twoHand){return twoHand;}
inline bool HideAmmo(bool opticalActive){return opticalActive;}
// The Katorga plate supersedes the old plain wrist strip. Never stack both
// health readouts in the same eye; the plate owns the live health presentation.
inline bool ShowLegacyWristHealth(bool legacyEnabled, bool plateEnabled) {
    return legacyEnabled && !plateEnabled;
}
constexpr int kScalePct = 88;
constexpr unsigned kPlateAlphaPct = 82;
constexpr unsigned kDigitAlphaPct = 94;

inline bool Visible(bool enabled, bool vr, bool duplicated, bool gameplay,
                    bool paused, bool menu, bool panel, bool cinematic) {
    return enabled && vr && duplicated && gameplay &&
           !paused && !menu && !panel && !cinematic;
}

inline int ScaledExtent(int value) {
    if (value <= 0) return value;
    return std::max(1, (value * kScalePct + 50) / 100);
}

inline std::uint32_t WithAlphaPct(std::uint32_t argb, unsigned pct) {
    if (pct > 100) pct = 100;
    const std::uint32_t alpha = (pct * 255u + 50u) / 100u;
    return (argb & 0x00FFFFFFu) | (alpha << 24);
}
}
