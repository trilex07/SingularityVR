#pragma once
// MAIN diegetic weapon HUD — weapon-mounted 3D diegetic display.
// Pure math + gates, no D3D, no engine state, so it is L1-testable (the same
// discipline as ammo_layout.h).
//
// Runtime representation:
// - Captured live weapon transform: skin (bone) * local (actor) * controller (GunC)
// - 3D fixture geometry: milled steel housing box (bezel + flanks) + recessed display bed
// - Dynamic Soviet E99 phosphor readout (clip / reserve) located on the 3D screen plane
// - Stereo eye projection: eye clip via game camera view-projection matrices (with ApplyEyeRemap)
// - Gate [Render] DiegeticWeaponHud, default OFF; legacy plates back off on 2-eye draw.
//
// Measured source evidence (side/diegetic-hud-native @ ff29efd,
// tools/hud/prepare_diegetic_models_v2.py, run configs 'centurion', 'valkyrie', 'spikeshot'):
//   - Centurion: ch_inview_pistol_mesh.psk, center=(0.0, 115.3, 43.5) UU, size=(2.6, 0.45, 1.8) UU, pitch 18 deg, rootSlot 1 ('06/24')
//   - Valkyrie:  ch_inview_machinegun_mesh.psk, center=(0.0, 117.2, 35.0) UU, size=(2.8, 0.45, 1.9) UU, pitch 20 deg, rootSlot 0 ('30/120')
//   - Spikeshot: ch_inview_shotgun_mesh.psk, center=(0.0, 116.8, 42.0) UU, size=(2.8, 0.45, 1.9) UU, pitch 15 deg, rootSlot 2 ('08/32')
//   - 1 UU = 0.0254 m.
#include <cmath>
#include <cstdint>
#include <algorithm>
#ifndef _WINDOWS_
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace diegetichud {

inline constexpr float kUUMetres = 0.0254f;   // UE unit scale
inline constexpr float kHousingWUU = 2.6f;    // measured Centurion fixture
inline constexpr float kHousingHUU = 0.45f;
inline constexpr float kMountPitchDeg = 18.0f;
// Measured Centurion fixture Y (the "on top of the receiver" baseline the
// hand-anchor offset was calibrated against in the first candidate).
inline constexpr float kBaselineYUU = 115.3f;

inline float HousingWidthMetres() { return kHousingWUU * kUUMetres; }  // 0.06604
inline float HousingHeightMetres() { return kHousingHUU * kUUMetres; } // 0.01143

// Per-weapon mounts from the measured fixture builders
// (side/diegetic-hud-native @ ff29efd, tools/hud/prepare_diegetic_models_v2.py).
struct WeaponMount {
    const char* name;
    long verts;          // first-person gun mesh catalog id (evidence)
    int plate;           // accepted legacy plate index this counter replaces (0=Centurion, 1=Valkyrie, 2=Spikeshot)
    float cyUU;          // fixture centre height (top-strap line)
    float wUU, hUU, dUU; // housing box in UU (width across gun, height normal, depth along screen)
    float pitchDeg;      // pitch angle rearward towards shooter (degrees)
    int clipPad;         // zero-padded clip glyph count ('06','30','08')
    int resPad;          // zero-padded reserve width ('24','120','32')
    float cxUU;          // fixture centre X in model space (lateral)
    float czUU;          // fixture centre Z in model space (vertical)
    int rootSlot;        // bone palette slot index for root bone (Centurion 1, Valkyrie 0, Spikeshot 2)
};

// Measured values (do not invent a second source):
//  Centurion: ch_inview_pistol_mesh.psk     (0.0,115.3,43.5) size(2.6,0.45,1.8) pitch 18 rootSlot 1 '06/24'
//  Valkyrie : ch_inview_machinegun_mesh.psk (0.0,117.2,35.0) size(2.8,0.45,1.9) pitch 20 rootSlot 0 '30/120'
//  Spikeshot: ch_inview_shotgun_mesh.psk    (0.0,116.8,42.0) size(2.8,0.45,1.9) pitch 15 rootSlot 2 '08/32'
inline const WeaponMount* MountForVerts(long v) {
    static const WeaponMount kMounts[] = {
        { "centurion", 5799,  0, 115.3f, 2.6f, 0.45f, 1.8f, 18.0f, 2, 2, 0.0f, 43.5f, 1 },
        { "valkyrie",  11387, 1, 117.2f, 2.8f, 0.45f, 1.9f, 20.0f, 2, 3, 0.0f, 35.0f, 0 },
        { "spikeshot", 8297,  2, 116.8f, 2.8f, 0.45f, 1.9f, 15.0f, 2, 2, 0.0f, 42.0f, 2 },
    };
    for (const auto& m : kMounts) if (m.verts == v) return &m;
    return nullptr; // TMD / seeker / unknown: no diegetic counter
}

// Hand-anchor lift above the calibrated 0.03 m baseline: the per-weapon
// fixture centre difference to the Centurion baseline, converted to metres.
inline float MountAnchorLiftM(const WeaponMount& m) {
    return (m.cyUU - kBaselineYUU) * kUUMetres;
}

// Atlas cells in px of diegetic_hud_atlas.png (1024x512; source JSON).
struct Cell { float x, y, w, h; };
inline float AtlasW() { return 1024.0f; }
inline float AtlasH() { return 512.0f; }
inline Cell BezelBg() { return Cell{ 0.0f, 0.0f, 256.0f, 128.0f }; }
inline Cell ScreenBg() { return Cell{ 264.0f, 0.0f, 256.0f, 128.0f }; }
inline Cell Slash() { return Cell{ 926.0f, 136.0f, 52.0f, 120.0f }; }
// cyan row y=136, amber row y=266; each glyph column is +82 px, 72x120.
inline Cell Digit(bool amber, int d) {
    int i = (d < 0 || d > 9) ? 0 : d;
    return amber ? Cell{ 106.0f + 82.0f * i, 266.0f, 72.0f, 120.0f }
                 : Cell{ 106.0f + 82.0f * i, 136.0f, 72.0f, 120.0f };
}

// Low magazine threshold: <=2 rounds remaining switches clip digits to amber row.
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

// World-anchored width from the per-eye px-per-metre probe (legacy 2D fallback sizing).
inline float PanelWidthPx(float pxPerMetre, float renderHeight, float housingWUU) {
    if (!std::isfinite(pxPerMetre) || !(pxPerMetre > 1.0f)) return 0.0f;
    if (!(renderHeight >= 64.0f)) return 0.0f;
    const float maximum = renderHeight * 0.12f;
    const float width = housingWUU * kUUMetres * pxPerMetre * 3.0f;
    if (width <= 26.0f) return 26.0f;
    if (width >= maximum) return maximum;
    return width;
}
inline float PanelWidthPx(float pxPerMetre, float renderHeight) {
    return PanelWidthPx(pxPerMetre, renderHeight, kHousingWUU);
}

struct Quad { float x0, y0, x1, y1, u0, v0, u1, v1; };

inline int Power10(int p) { int v = 1; while (p-- > 0) v *= 10; return v; }

// 2D screen-space layout retained for legacy fallback / L1 tests.
inline int LayoutCounter(int clip, int reserve, float cx, float cy, float w,
    float h, Quad* out, int maxQuads, int resPad = 2) {
    if (!out || maxQuads <= 0) return 0;
    if (!(w >= 8.0f) || !(h >= 4.0f)) return 0;
    const Cell sc = ScreenBg();
    Quad& bg = out[0];
    bg.x0 = cx - w * 0.5f; bg.y0 = cy - h * 0.5f;
    bg.x1 = cx + w * 0.5f; bg.y1 = cy + h * 0.5f;
    bg.u0 = sc.x / AtlasW(); bg.v0 = sc.y / AtlasH();
    bg.u1 = (sc.x + sc.w) / AtlasW(); bg.v1 = (sc.h + sc.y) / AtlasH();
    if (clip < 0 || reserve < 0) return 1;
    const bool low = clip <= 2;
    const float bandH = 0.52f * h;
    const float glyphW = bandH * 72.0f / 120.0f;
    const float slashW = bandH * 52.0f / 120.0f;
    const float gap = 0.05f * w;
    auto countDigits = [](int v) { int n = 1; while (v >= 10) { v /= 10; ++n; } return n; };
    const int nc = countDigits(clip) < 2 ? 2 : countDigits(clip);
    const int nr = countDigits(reserve) < resPad ? resPad : countDigits(reserve);
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
        const int d = clip >= t ? (clip / t) % 10 : 0;
        push(adv + glyphW * 0.5f, Digit(low, d));
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

inline void ClampCentre(float& cx, float w, float backW) {
    const float half = backW * 0.5f;
    const float m = w * 0.5f + 1.0f;
    if (!std::isfinite(cx)) { cx = half; return; }
    if (cx < m) cx = m;
    if (cx > backW - m) cx = backW - m;
}

// ----------------------------------------------------------------------------
// 3D Geometry Representation & Live Transform Pipeline
// ----------------------------------------------------------------------------

struct V3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;
};
inline V3 operator+(const V3& a, const V3& b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
inline V3 operator-(const V3& a, const V3& b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
inline V3 operator*(const V3& a, float s) { return { a.x * s, a.y * s, a.z * s }; }
inline V3 operator*(float s, const V3& a) { return { a.x * s, a.y * s, a.z * s }; }

// Converts a point or vector from raw ActorX PSK space (PNTS0000) into
// Singularity's native cooked GPU vertex stream space (where Y is negated:
// native_pos = psk_pos * [1, -1, 1], proven in tools/reload/audit_next_weapon_space.py).
inline V3 ToNativeVertexSpace(const V3& pskPt) {
    return { pskPt.x, -pskPt.y, pskPt.z };
}

struct Quad3D {
    V3 p[4];            // 4 corners in native GPU weapon model space (0: top-left, 1: top-right, 2: bot-right, 3: bot-left)
    float u0 = 0.0f, v0 = 0.0f, u1 = 0.0f, v1 = 0.0f;
    uint32_t color = 0xFFFFFFFF;
};

// 4x4 Row-major Affine Matrix (matches armbind::Mat binary layout)
struct Mat4 {
    float m[4][4]{};
};

inline V3 Point(const V3& v, const Mat4& mat) {
    return {
        v.x * mat.m[0][0] + v.y * mat.m[1][0] + v.z * mat.m[2][0] + mat.m[3][0],
        v.x * mat.m[0][1] + v.y * mat.m[1][1] + v.z * mat.m[2][1] + mat.m[3][1],
        v.x * mat.m[0][2] + v.y * mat.m[1][2] + v.z * mat.m[2][2] + mat.m[3][2]
    };
}

inline bool Finite(const Mat4& a) {
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            if (!std::isfinite(a.m[r][c])) return false;
    return true;
}

// Thread-safe storage for the live captured weapon transform (seqlock)
struct LiveTransform {
    volatile LONG seq = 0;
    volatile LONG valid = 0;
    long verts = 0;
    uint64_t tick = 0;
    unsigned frame = 0;
    Mat4 chain{};
};

inline LiveTransform& GlobalLiveTransform() {
    static LiveTransform g_tf;
    return g_tf;
}

inline void RecordLiveTransform(long verts, const Mat4& chain, uint64_t tick, unsigned frame) {
    if (!Finite(chain)) return;
    auto& tf = GlobalLiveTransform();
    const LONG s0 = InterlockedIncrement(&tf.seq);
    (void)s0;
    MemoryBarrier();
    tf.verts = verts;
    tf.tick = tick;
    tf.frame = frame;
    tf.chain = chain;
    InterlockedExchange(&tf.valid, 1);
    MemoryBarrier();
    InterlockedIncrement(&tf.seq);
}

inline bool GetLiveTransform(long verts, uint64_t now, unsigned frame, Mat4& out) {
    auto& tf = GlobalLiveTransform();
    for (int attempt = 0; attempt < 4; ++attempt) {
        const LONG s1 = InterlockedCompareExchange(&tf.seq, 0, 0);
        if (s1 & 1) continue; // write in progress
        MemoryBarrier();
        const LONG v = InterlockedCompareExchange(&tf.valid, 0, 0);
        const long curVerts = tf.verts;
        const uint64_t curTick = tf.tick;
        const unsigned curFrame = tf.frame;
        const Mat4 curChain = tf.chain;
        MemoryBarrier();
        const LONG s2 = InterlockedCompareExchange(&tf.seq, 0, 0);
        if (s1 != s2) continue;
        if (!v || curVerts != verts) return false;
        if (frame > 0 && curFrame > 0) {
            const unsigned diff = (frame >= curFrame) ? (frame - curFrame) : (curFrame - frame);
            if (diff > 2) return false;
        }
        if (now >= curTick) {
            if (now - curTick > 100) return false;
        } else {
            if (curTick - now > 50) return false;
        }
        if (!Finite(curChain)) return false;
        out = curChain;
        return true;
    }
    return false;
}

// Screen-space viewport visibility check for a projected eye half.
// Ensures that a quad submission is only counted as visible when its projected
// bounding box is finite, has positive pixel extent, and lies inside the eye viewport.
inline bool EyeBoundsVisible(float minX, float minY, float maxX, float maxY,
                             int eye, unsigned backW, unsigned backH) {
    if (!std::isfinite(minX) || !std::isfinite(minY) ||
        !std::isfinite(maxX) || !std::isfinite(maxY)) return false;
    if (backW < 8 || backH < 8 || eye < 0 || eye > 1) return false;
    const float wPx = maxX - minX;
    const float hPx = maxY - minY;
    if (wPx < 2.0f || hPx < 2.0f || wPx > (float)backW || hPx > (float)backH) return false;
    const float eyeX0 = eye == 0 ? 0.0f : (float)(backW / 2);
    const float eyeX1 = eye == 0 ? (float)(backW / 2) : (float)backW;
    const float eyeY0 = 0.0f;
    const float eyeY1 = (float)backH;
    // Require intersection with the eye's viewport half
    const float ix0 = (std::max)(minX, eyeX0);
    const float iy0 = (std::max)(minY, eyeY0);
    const float ix1 = (std::min)(maxX, eyeX1);
    const float iy1 = (std::min)(maxY, eyeY1);
    return (ix1 - ix0) >= 2.0f && (iy1 - iy0) >= 2.0f;
}

// Generates 3D fixture geometry in native GPU weapon model space:
// - 4 Housing Box Flanks (solid gunmetal steel dropping into weapon receiver)
// - 1 Bezel Top Face (bezel_bg rim with rivets)
// - 1 Recessed Screen Bed (screen_bg dark phosphor CRT glass)
// - Dynamic numerals + slash quads positioned on the 3D screen plane
// Returns the number of quads written into out (<= maxQuads).
inline int BuildFixtureQuads3D(const WeaponMount& mo, int clip, int reserve,
                               Quad3D* out, int maxQuads) {
    if (!out || maxQuads < 12) return 0;

    const float rad = mo.pitchDeg * (3.141592653589793f / 180.0f);
    const float cos_p = std::cos(rad);
    const float sin_p = std::sin(rad);

    // Orthonormal basis in ActorX PSK space (prepare_diegetic_models_v2.py):
    //   right_psk = (1, 0, 0)
    //   fwd_psk   = (0, sin_p, cos_p)
    //   norm_psk  = (0, cos_p, -sin_p)
    // Converted to native cooked GPU vertex stream space via ToNativeVertexSpace
    // (Y negated: native_pos = psk_pos * [1, -1, 1]):
    const V3 right = ToNativeVertexSpace({ 1.0f, 0.0f, 0.0f });
    const V3 fwd   = ToNativeVertexSpace({ 0.0f, sin_p, cos_p });
    const V3 norm  = ToNativeVertexSpace({ 0.0f, cos_p, -sin_p });

    const V3 center = ToNativeVertexSpace({ mo.cxUU, mo.cyUU, mo.czUU });
    const float rx = mo.wUU * 0.5f;
    const float ry = mo.hUU * 0.5f;
    const float rz = mo.dUU * 0.5f;

    const V3 r = right * rx;
    const V3 u = norm * ry;
    const V3 f = fwd * rz;

    // 8 corners of the 3D housing box
    // Top corners (+u outward towards shooter)
    // Note: p0..p1 is +right, p3..p0 is -fwd (down the screen), p0..p3 is +fwd (up the screen).
    // For top-left (u0,v0) -> top-right (u1,v0) -> bot-right (u1,v1) -> bot-left (u0,v1),
    // top of display is +f (p3, p2) and bottom of display is -f (p0, p1).
    const V3 p0 = center - r - f + u; // bot-left on display (+u)
    const V3 p1 = center + r - f + u; // bot-right on display (+u)
    const V3 p2 = center + r + f + u; // top-right on display (+u)
    const V3 p3 = center - r + f + u; // top-left on display (+u)

    // Bottom corners (-u into receiver)
    const V3 p4 = center - r - f - u; // bot-left base (-u)
    const V3 p5 = center + r - f - u; // bot-right base (-u)
    const V3 p6 = center + r + f - u; // top-right base (-u)
    const V3 p7 = center - r + f - u; // top-left base (-u)

    int n = 0;

    // Dark gunmetal steel tint for housing box flanks
    const uint32_t flankCol = 0xFF202630;
    const float f_u0 = 4.0f / AtlasW(), f_v0 = 4.0f / AtlasH();
    const float f_u1 = 16.0f / AtlasW(), f_v1 = 16.0f / AtlasH();

    // 1. Four 3D Box Flanks (solid steel sides dropping into weapon)
    out[n++] = Quad3D{ { p0, p1, p5, p4 }, f_u0, f_v0, f_u1, f_v1, flankCol };
    out[n++] = Quad3D{ { p2, p3, p7, p6 }, f_u0, f_v0, f_u1, f_v1, flankCol };
    out[n++] = Quad3D{ { p3, p0, p4, p7 }, f_u0, f_v0, f_u1, f_v1, flankCol };
    out[n++] = Quad3D{ { p1, p2, p6, p5 }, f_u0, f_v0, f_u1, f_v1, flankCol };

    // 2. Bezel Top Face (chamfered rim + rivets): top-left p3, top-right p2, bot-right p1, bot-left p0
    const Cell bz = BezelBg();
    const float b_u0 = bz.x / AtlasW(), b_v0 = bz.y / AtlasH();
    const float b_u1 = (bz.x + bz.w) / AtlasW(), b_v1 = (bz.y + bz.h) / AtlasH();
    out[n++] = Quad3D{ { p3, p2, p1, p0 }, b_u0, b_v0, b_u1, b_v1, 0xFFFFFFFF };

    // 3. Recessed Cathode Display Bed
    // Sits slightly proud of bezel top to prevent depth fighting
    const V3 dispCenter = center + u + norm * 0.005f;
    const float sw = mo.wUU * 0.82f;
    const float sh = mo.dUU * 0.78f;
    const V3 sr = right * (sw * 0.5f);
    const V3 sf = fwd * (sh * 0.5f);

    const Cell sc = ScreenBg();
    const float s_u0 = sc.x / AtlasW(), s_v0 = sc.y / AtlasH();
    const float s_u1 = (sc.x + sc.w) / AtlasW(), s_v1 = (sc.y + sc.h) / AtlasH();

    const V3 s_tl = dispCenter - sr + sf; // top-left (+sf)
    const V3 s_tr = dispCenter + sr + sf; // top-right (+sf)
    const V3 s_br = dispCenter + sr - sf; // bot-right (-sf)
    const V3 s_bl = dispCenter - sr - sf; // bot-left (-sf)
    out[n++] = Quad3D{ { s_tl, s_tr, s_br, s_bl }, s_u0, s_v0, s_u1, s_v1, 0xFFFFFFFF };

    if (clip < 0 || reserve < 0) return n; // unknown -> housing & screen only

    // 4. Dynamic Numerals and Slash on the Screen Plane
    const bool low = LowState(clip);
    const V3 textCenter = dispCenter + norm * 0.003f;
    const float bandH = sh * 0.54f;
    const float glyphW = bandH * (72.0f / 120.0f);
    const float slashW = bandH * (52.0f / 120.0f);
    const float gap = sw * 0.04f;

    auto countDigits = [](int v) { int d = 1; while (v >= 10) { v /= 10; ++d; } return d; };
    const int nc = countDigits(clip) < mo.clipPad ? mo.clipPad : countDigits(clip);
    const int nr = countDigits(reserve) < mo.resPad ? mo.resPad : countDigits(reserve);
    const float totalW = nc * glyphW + gap + slashW + gap + nr * glyphW;

    float adv = -totalW * 0.5f;

    auto pushGlyph = [&](float centerOffset, const Cell& c) {
        if (n >= maxQuads) return;
        const float gw = (c.w / c.h) * bandH;
        const V3 qc = textCenter + right * centerOffset;
        const V3 qr = right * (gw * 0.5f);
        const V3 qf = fwd * (bandH * 0.5f);
        const float u0 = c.x / AtlasW();
        const float v0 = c.y / AtlasH();
        const float u1 = (c.x + c.w) / AtlasW();
        const float v1 = (c.y + c.h) / AtlasH();
        // Top of glyph is +qf (v0), bottom of glyph is -qf (v1)
        out[n++] = Quad3D{
            { qc - qr + qf, qc + qr + qf, qc + qr - qf, qc - qr - qf },
            u0, v0, u1, v1, 0xFFFFFFFF
        };
    };

    // Clip digits (amber if low, e.g. <= 2)
    for (int i = 0; i < nc; ++i, adv += glyphW) {
        const int t = Power10(nc - 1 - i);
        const int d = clip >= t ? (clip / t) % 10 : 0;
        pushGlyph(adv + glyphW * 0.5f, Digit(low, d));
    }

    // Slash
    pushGlyph(adv + gap + slashW * 0.5f, Slash());
    adv += 2.0f * gap + slashW;

    // Reserve digits
    for (int i = 0; i < nr; ++i, adv += glyphW) {
        const int t = Power10(nr - 1 - i);
        const int d = reserve >= t ? (reserve / t) % 10 : 0;
        pushGlyph(adv + glyphW * 0.5f, Digit(false, d));
    }

    return n;
}

} // namespace diegetichud
