#pragma once
// MAIN diegetic weapon HUD — milestone 1: Centurion '06/24' counter.
// Pure math + gates, no D3D, no engine state, so it is L1-testable (the same
// discipline as ammo_layout.h).
//
// Runtime representation: a few textured quads drawn per eye inside the
// VERIFIED ammo-plate pipeline (aim-hand anchor, per-eye projection, world
// anchored px-per-metre probe, own state block, full state restore). This is
// deliberately NOT a mesh replacement: no GLB/OBJ is loaded at runtime; the
// housing and glyph art ship as one small runtime atlas PNG (1024x512)
// deployed to Binaries/vr-assets/hud/diegetic/diegetic_hud_atlas.png. Live
// values only come from the native HUD sampler (mClipAmmo/mBinAmmo, proven by
// the round-9 probe); unknown values hide the counter, never a guess.
//
// Measured source evidence (side/diegetic-hud-native @ ff29efd,
// tools/hud/prepare_diegetic_models_v2.py, run config 'centurion'):
//   - PSK: ch_inview_pistol_mesh.psk
//   - fixture center=(0.0, 115.3, 43.5) UU, size=(2.6, 0.45, 1.8) UU,
//     pitch 18 deg; builder axes right=(1,0,0), fwd=(0,sin18,cos18),
//     norm=(0,cos18,-sin18) (recessed display bed faces +norm).
//   - 1 UU = 0.0254 m => housing on body: 6.60 x 1.14 cm, top-strap mount.
// Phase 1 renders a 2:1 diegetic screen panel (screen_bg cell aspect) pinned
// at the weapon anchor, world-scaled by these measured dimensions; exact
// on-body recessed-strip placement needs the weapon world matrix capture and
// is a follow-up after the physical grade. Gate [Render] DiegeticWeaponHud,
// default OFF; the Centurion ammo plate backs off while this draws (both
// switches stay independent for rollback).
#include <cmath>

namespace diegetichud {

inline constexpr float kUUMetres = 0.0254f;   // UE unit scale (evidence above)
inline constexpr float kHousingWUU = 2.6f;    // measured Centurion fixture
inline constexpr float kHousingHUU = 0.45f;
inline constexpr float kMountPitchDeg = 18.0f;

inline float HousingWidthMetres() { return kHousingWUU * kUUMetres; }  // 0.06604
inline float HousingHeightMetres() { return kHousingHUU * kUUMetres; } // 0.01143

// Atlas cells in px of diegetic_hud_atlas.png (1024x512; source JSON).
struct Cell { float x, y, w, h; };
inline float AtlasW() { return 1024.0f; }
inline float AtlasH() { return 512.0f; }
inline Cell ScreenBg() { return Cell{ 264.0f, 0.0f, 256.0f, 128.0f }; }
inline Cell Slash() { return Cell{ 926.0f, 136.0f, 52.0f, 120.0f }; }
// cyan row y=136, amber row y=266; each glyph column is +82 px, 72x120.
inline Cell Digit(bool amber, int d) {
    int i = (d < 0 || d > 9) ? 0 : d;
    return amber ? Cell{ 106.0f + 82.0f * i, 266.0f, 72.0f, 120.0f }
                 : Cell{ 106.0f + 82.0f * i, 136.0f, 72.0f, 120.0f };
}

// Centurion magazine runs low at <=2 rounds (plate rule: red cover stays on
// while 3+ rounds remain). The counter flips to the amber glyph row there.
inline bool LowState(int clip) { return clip >= 0 && clip <= 2; }

// Full gate. Mirrors the accepted ammo-plate pipeline conditions plus its own
// defaults: hidden while the R3 wheel is open, while not in gameplay stereo,
// paused/menu/panel/cinematic, while no valid in-hands weapon or eye data, or
// while the live ammo values are unknown.
inline bool PanelVisible(bool flagOn, bool artReady, bool centurionInHands,
    bool wheelOpen, bool paused, bool menuOpen, bool panelOpen, bool cinematic,
    bool gameplay, bool inHands, bool handValid, bool haveEyes, bool ammoKnown) {
    return flagOn && artReady && centurionInHands && !wheelOpen && !paused &&
        !menuOpen && !panelOpen && !cinematic && gameplay && inHands &&
        handValid && haveEyes && ammoKnown;
}

// World-anchored width from the per-eye px-per-metre probe (same evidence
// shape as ammoplates::EyePlateWidth). Probe failure returns 0; the caller
// then uses its own bounded fallback instead of inheriting the other eye.
inline float PanelWidthPx(float pxPerMetre, float renderHeight) {
    if (!std::isfinite(pxPerMetre) || !(pxPerMetre > 1.0f)) return 0.0f;
    if (!(renderHeight >= 64.0f)) return 0.0f;
    const float maximum = renderHeight * 0.12f;
    const float width = HousingWidthMetres() * pxPerMetre * 3.0f; // legible pin
    if (width <= 26.0f) return 26.0f;
    if (width >= maximum) return maximum;
    return width;
}

struct Quad { float x0, y0, x1, y1, u0, v0, u1, v1; };

inline int Power10(int p) { int v = 1; while (p-- > 0) v *= 10; return v; }

// Compose the counter at centre (cx,cy) with panel pixel size (w x h):
// quad 0 = screen_bg panel; then nc clip glyphs, one slash, nr reserve glyphs
// centred in the band (band = 52% of panel height; glyph aspect from cells).
// Clip is zero-padded to 2 ('06'); reserve to 2 or 3 ('24', '120'). Low mag
// switches clip glyphs to the amber row. Unknown values (negative) draw the
// panel without digits so the runtime can hide instead of presenting garbage.
// Returns the number of quads written (<= maxQuads); 0 only on invalid input.
inline int LayoutCounter(int clip, int reserve, float cx, float cy, float w,
    float h, Quad* out, int maxQuads) {
    if (!out || maxQuads <= 0) return 0;
    if (!(w >= 8.0f) || !(h >= 4.0f)) return 0;
    const Cell sc = ScreenBg();
    Quad& bg = out[0];
    bg.x0 = cx - w * 0.5f; bg.y0 = cy - h * 0.5f;
    bg.x1 = cx + w * 0.5f; bg.y1 = cy + h * 0.5f;
    bg.u0 = sc.x / AtlasW(); bg.v0 = sc.y / AtlasH();
    bg.u1 = (sc.x + sc.w) / AtlasW(); bg.v1 = (sc.y + sc.h) / AtlasH();
    if (clip < 0 || reserve < 0) return 1;
    const bool low = clip <= 2;
    const float bandH = 0.52f * h;
    const float glyphW = bandH * 72.0f / 120.0f;
    const float slashW = bandH * 52.0f / 120.0f;
    const float gap = 0.05f * w;
    auto countDigits = [](int v) { int n = 1; while (v >= 10) { v /= 10; ++n; } return n; };
    const int nc = countDigits(clip) < 2 ? 2 : countDigits(clip);
    const int nr = countDigits(reserve) < 2 ? 2 : countDigits(reserve);
    const float total = nc * glyphW + gap + slashW + gap + nr * glyphW;
    float adv = cx - total * 0.5f;
    int n = 1;
    auto push = [&](float centre, const Cell& c) {
        if (n >= maxQuads) return;
        Quad& q = out[n++];
        q.x0 = centre - c.w * (bandH / c.h) * 0.5f;
        q.x1 = centre + c.w * (bandH / c.h) * 0.5f;
        q.y0 = cy - bandH * 0.5f; q.y1 = cy + bandH * 0.5f;
        q.u0 = c.x / AtlasW(); q.v0 = c.y / AtlasH();
        q.u1 = (c.x + c.w) / AtlasW(); q.v1 = (c.y + c.h) / AtlasH();
    };
    for (int i = 0; i < nc; ++i, adv += glyphW) {
        const int t = Power10(nc - 1 - i);
        const int d = clip >= t ? (clip / t) % 10 : 0; // zero-padded ('06')
        push(adv + glyphW * 0.5f, Digit(low, d));      // amber row while low
    }
    push(adv + gap + slashW * 0.5f, Slash());
    adv += 2.0f * gap + slashW;
    for (int i = 0; i < nr; ++i, adv += glyphW) {
        const int t = Power10(nr - 1 - i);
        const int d = reserve >= t ? (reserve / t) % 10 : 0;
        push(adv + glyphW * 0.5f, Digit(false, d));
    }
    return n;
}

// Keep the panel inside this eye's half of the back buffer (plate rule).
inline void ClampCentre(float& cx, float w, float backW) {
    const float half = backW * 0.5f;
    const float m = w * 0.5f + 1.0f;
    if (!std::isfinite(cx)) { cx = half; return; }
    if (cx < m) cx = m;
    if (cx > backW - m) cx = backW - m;
}

} // namespace diegetichud
