#pragma once
#include <cmath>

// Resolution-independent VR prompt/icon placement (isolated helper).
//
// Context (prompt research on branch feature/psvr2-polish-parallel):
//   - PlayStation icons DO render (atlas + overlay work), but the loading
//     legend sat too low: it used blind backbuffer fractions (.83H/.90H,
//     now .72H in the sibling's working tree) with no FOV/crop/safe-area
//     awareness, so the bottom rows fall outside the comfortable headset
//     frustum on the 2684px SBS target and collide with the menu quad.
//   - PAD2/PAD3 text does NOT exist in game data (verified negative across
//     Coalesced_INT.bin, Singularity.exe, RvGame.u/Engine.u/Core.u/GFxUI.u
//     and the SF movies). Native gamepad prompts are Xbox button-font
//     glyphs (w/x/y/z/L/C via RUIDataStore_StringAliasMap) and [token]
//     display strings resolved exe-side. Do NOT build token replacement on
//     the PAD assumption — the overlay stays a legend until the real
//     gamepad-path [token] rendering is confirmed via screenshot.
//   - KHARVOX neither replaces native prompts nor overlays icons (verified
//     negative); no transferable prompt architecture exists there.
//
// This header owns ONLY the placement math so the main agent can connect it
// to DrawVrPromptLegend without touching menu capture or laser paths:
//   - SafeArea(): visible fraction of a full-height frame from the rendered
//     half-FOV tangents vs the headset's visible tangents (anything past
//     the visible frustum is peripheral/cut — that is why .83H was "too low").
//   - LegendRow(): icon/label row Y for an N-row bottom-anchored legend
//     inside the safe area, resolution-independent (no pixel floors that
//     break at low H; caller scales icon size from row height).
//   - EyeBase(): per-eye SBS x origin for duplicated overlay geometry.
// Pure logic, no production dependency. Gameplay HUD prompts only.

namespace promptplace {

struct SafeArea {
    float visibleFraction = 1.0f; // of full frame height, centered
    float topY = 0.0f; // normalized safe-area top (0..1 from frame top)
    float bottomY = 1.0f; // normalized safe-area bottom
};

// renderedTan = tan(half-FOV) of the rendered frame (per axis relevant to
// the layout direction); visibleTan = tan(half-FOV) actually comfortably
// visible in the headset. Returns the centered visible fraction <= 1.
inline bool SafeAreaFromFov(float renderedTanHalf, float visibleTanHalf, SafeArea& out) {
    if (!std::isfinite(renderedTanHalf) || !std::isfinite(visibleTanHalf) || renderedTanHalf <= 0 ||
        visibleTanHalf <= 0)
        return false;
    SafeArea s;
    s.visibleFraction = visibleTanHalf >= renderedTanHalf ? 1.0f : visibleTanHalf / renderedTanHalf;
    const float margin = (1.0f - s.visibleFraction) * 0.5f;
    s.topY = margin;
    s.bottomY = 1.0f - margin;
    if (!std::isfinite(s.topY) || !std::isfinite(s.bottomY)) return false;
    out = s;
    return true;
}

// Bottom-anchored legend row: row 0 is the lowest row. rowHeight and margin
// are fractions of the SAFE-area height (not the frame), so the legend
// never drifts into the cut region at any resolution.
inline bool LegendRowY(const SafeArea& area, unsigned row, float rowHeight, float margin,
                       float& outY) {
    if (!std::isfinite(rowHeight) || !std::isfinite(margin) || rowHeight <= 0 || rowHeight > 1 ||
        margin < 0 || area.bottomY <= area.topY)
        return false;
    const float safeH = area.bottomY - area.topY;
    outY = area.bottomY - margin * safeH - (row + 0.5f) * rowHeight * safeH;
    return std::isfinite(outY) && outY >= area.topY && outY <= area.bottomY;
}

// Per-eye SBS x origin: full SBS width, two half-width eyes, fractional
// inset within the eye (matches the existing overlay duplication pattern).
inline bool EyeBaseX(float sbsWidth, unsigned eye, float insetFraction, float& outX) {
    if (!std::isfinite(sbsWidth) || sbsWidth < 2 || eye > 1 || !std::isfinite(insetFraction) ||
        insetFraction < 0 || insetFraction > 1)
        return false;
    const float eyeW = sbsWidth * 0.5f;
    outX = eye * eyeW + eyeW * insetFraction;
    return std::isfinite(outX);
}

} // namespace promptplace
