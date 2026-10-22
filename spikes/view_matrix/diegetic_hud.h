#pragma once
// Singularity VR — Minimalist Diegetic In-World HUD System
// Implements true diegetic 3D HUDs physically mounted onto weapons and gloves.
//
// Soviet 1950s / E99 Sci-Fi Aesthetic:
// - Weapon: Compact rugged milled-metal ammo counter mounted on the receiver,
//   displaying clip ammo and reserve ammo in glowing E99 phosphor nixie/7-segment digits.
//   Color dynamics: E99 Electric Cyan (normal) -> Amber (low <= 2) -> Crimson (empty).
// - Wrist / Glove: Sleek curved bio-monitor mounted flush on the left wrist/TMD,
//   displaying an authentic 10-segment E99 phosphor energy gauge.
//   Color dynamics: E99 Cyan (>= 70%) -> Amber (30%-69%) -> Pulsing Alarm Red (< 30%).
//
// Pure math & geometry: completely unit-testable without D3D device or engine globals.

#include <cmath>
#include <cstdint>
#include <algorithm>

namespace diegetichud {

// ---------------------------------------------------------------------------
// Atlas Coordinates (512 x 256 diegetic_hud_atlas.png)
// ---------------------------------------------------------------------------
struct AtlasRect {
    float u0, v0, u1, v1;
};

inline AtlasRect MakeRect(int x, int y, int w, int h, float totalW = 512.0f, float totalH = 256.0f) {
    return AtlasRect{
        (float)x / totalW,
        (float)y / totalH,
        (float)(x + w) / totalW,
        (float)(y + h) / totalH
    };
}

// Digits 0 through 9
inline AtlasRect DigitRect(int d) {
    if (d < 0 || d > 9) d = 0;
    if (d <= 6) {
        // Row 1: x = 6 + d * 46, y = 76, w = 42, h = 68
        return MakeRect(6 + d * 46, 76, 42, 68);
    } else {
        // Row 2: x = 6 + (d - 7) * 46, y = 166, w = 42, h = 68
        return MakeRect(6 + (d - 7) * 46, 166, 42, 68);
    }
}

inline AtlasRect SlashRect() {
    // Row 2, index 3: x = 6 + 3 * 46 = 144, y = 166, w = 42, h = 68
    return MakeRect(144, 166, 42, 68);
}

inline AtlasRect ExclaimRect() {
    // Row 2, index 4: x = 6 + 4 * 46 = 190, y = 166, w = 42, h = 68
    return MakeRect(190, 166, 42, 68);
}

inline AtlasRect DashRect() {
    // Row 2, index 5: x = 6 + 5 * 46 = 236, y = 166, w = 42, h = 68
    return MakeRect(236, 166, 42, 68);
}

inline AtlasRect BezelBgRect() {
    return MakeRect(0, 0, 128, 64);
}

inline AtlasRect ScreenBgRect() {
    return MakeRect(132, 0, 128, 64);
}

inline AtlasRect HealthSegFullRect() {
    return MakeRect(264, 0, 32, 48);
}

inline AtlasRect HealthSegEmptyRect() {
    return MakeRect(300, 0, 32, 48);
}

inline AtlasRect BioIconRect() {
    return MakeRect(336, 0, 48, 48);
}

inline AtlasRect MagIconRect() {
    return MakeRect(388, 0, 48, 48);
}

// ---------------------------------------------------------------------------
// E99 Aesthetic Colors (ARGB format)
// ---------------------------------------------------------------------------
inline uint32_t ColorE99Cyan(uint8_t alpha = 255) {
    return ((uint32_t)alpha << 24) | 0x0000E5FF;
}

inline uint32_t ColorAmberWarn(uint8_t alpha = 255) {
    return ((uint32_t)alpha << 24) | 0x00FF9100;
}

inline uint32_t ColorCrimsonAlert(uint8_t alpha = 255) {
    return ((uint32_t)alpha << 24) | 0x00FF1744;
}

inline uint32_t ColorBezelMetal(uint8_t alpha = 240) {
    return ((uint32_t)alpha << 24) | 0x00242C34;
}

inline uint32_t ColorScreenGlass(uint8_t alpha = 220) {
    return ((uint32_t)alpha << 24) | 0x000A1218;
}

inline uint32_t AmmoColor(int clip) {
    if (clip <= 0) return ColorCrimsonAlert();
    if (clip <= 2) return ColorAmberWarn();
    return ColorE99Cyan();
}

inline uint32_t HealthColor(float fraction) {
    if (fraction < 0.30f) return ColorCrimsonAlert();
    if (fraction < 0.70f) return ColorAmberWarn();
    return ColorE99Cyan();
}

// ---------------------------------------------------------------------------
// 3D Geometry Structures
// ---------------------------------------------------------------------------
struct Vertex3D {
    float x, y, z;
    uint32_t color;
    float u, v;
};

struct Quad3D {
    // 4 corners: 0=Top-Left, 1=Top-Right, 2=Bottom-Left, 3=Bottom-Right
    Vertex3D v[4];
};

// Weapon Mount Profile (positions in Unreal Units, 1 UU ~ 1.905 cm)
struct WeaponMountProfile {
    float localX;     // Lateral offset across the receiver
    float localY;     // Vertical height above gun pivot/grip
    float localZ;     // Longitudinal distance along barrel from pivot
    float pitchDeg;   // Backward tilt towards shooter's eye
    float widthUU;    // Physical display width
    float heightUU;   // Physical display height
};

inline WeaponMountProfile GetWeaponProfile(long verts) {
    // Centurion Revolver (catalog: 5799 verts)
    // Mounted flush on the top strap frame right above the cylinder
    if (verts == 5799) {
        return WeaponMountProfile{ 0.0f, 2.75f, 2.40f, -15.0f, 1.80f, 0.85f };
    }
    // Valkyrie AR9 Rifle (catalog: 11387 verts)
    // Mounted on the receiver top plate forward of the rear sight aperture
    if (verts == 11387) {
        return WeaponMountProfile{ 0.0f, 4.20f, 0.40f, -20.0f, 1.95f, 0.90f };
    }
    // Spikeshot Shotgun (catalog: 8297 verts)
    // Mounted on the receiver top rail
    if (verts == 8297) {
        return WeaponMountProfile{ 0.0f, 3.60f, 2.80f, -15.0f, 1.95f, 0.90f };
    }
    // Kasimov SNV-E99 Sniper Rifle (catalog: 11389 verts)
    // Mounted on the receiver base under the scope mount
    if (verts == 11389) {
        return WeaponMountProfile{ 0.0f, 3.40f, 1.20f, -15.0f, 1.85f, 0.85f };
    }
    // Default fallback
    return WeaponMountProfile{ 0.0f, 3.00f, 1.50f, -15.0f, 1.80f, 0.85f };
}

// Left Wrist / TMD Health Gauge Mount Profile
struct WristMountProfile {
    float localX;     // Across forearm
    float localY;     // Up from wrist bone
    float localZ;     // Forward along wrist/gauntlet
    float rollDeg;    // Inward tilt towards eyes when arm raised (~45 deg)
    float lengthUU;   // Length along wrist (~4.2 cm = 2.2 UU)
    float widthUU;    // Width across wrist (~1.3 cm = 0.7 UU)
};

inline WristMountProfile GetWristProfile() {
    // Positioned cleanly on the dorsal-inward wrist casing
    return WristMountProfile{ -0.6f, 1.8f, 0.5f, 40.0f, 2.30f, 0.75f };
}

// ---------------------------------------------------------------------------
// Matrix Mathematics
// ---------------------------------------------------------------------------
// Multiply local 3D point by 4x4 row-major matrix (row 0=Right, row 1=Forward, row 2=Up, row 3=Pos)
inline void TransformPoint(const float pt[3], const float M[4][4], float out[3]) {
    out[0] = pt[0] * M[0][0] + pt[1] * M[1][0] + pt[2] * M[2][0] + M[3][0];
    out[1] = pt[0] * M[0][1] + pt[1] * M[1][1] + pt[2] * M[2][1] + M[3][1];
    out[2] = pt[0] * M[0][2] + pt[1] * M[1][2] + pt[2] * M[2][2] + M[3][2];
}

// Transform entire 3D Quad by 4x4 matrix
inline void TransformQuad(const Quad3D& inQuad, const float M[4][4], Quad3D& outQuad) {
    for (int i = 0; i < 4; ++i) {
        float pt[3] = { inQuad.v[i].x, inQuad.v[i].y, inQuad.v[i].z };
        float res[3];
        TransformPoint(pt, M, res);
        outQuad.v[i].x = res[0];
        outQuad.v[i].y = res[1];
        outQuad.v[i].z = res[2];
        outQuad.v[i].color = inQuad.v[i].color;
        outQuad.v[i].u = inQuad.v[i].u;
        outQuad.v[i].v = inQuad.v[i].v;
    }
}

// Construct a planar quad given a center, right vector, up vector, and half extents
inline Quad3D BuildPlanarQuad(const float center[3], const float right[3], const float up[3],
                              float halfW, float halfH, const AtlasRect& uv, uint32_t color) {
    Quad3D q;
    // 0: Top-Left  = center - right*halfW + up*halfH
    // 1: Top-Right = center + right*halfW + up*halfH
    // 2: Bot-Left  = center - right*halfW - up*halfH
    // 3: Bot-Right = center + right*halfW - up*halfH
    for (int i = 0; i < 3; ++i) {
        q.v[0].x = center[0] - right[0] * halfW + up[0] * halfH;
        q.v[0].y = center[1] - right[1] * halfW + up[1] * halfH;
        q.v[0].z = center[2] - right[2] * halfW + up[2] * halfH;

        q.v[1].x = center[0] + right[0] * halfW + up[0] * halfH;
        q.v[1].y = center[1] + right[1] * halfW + up[1] * halfH;
        q.v[1].z = center[2] + right[2] * halfW + up[2] * halfH;

        q.v[2].x = center[0] - right[0] * halfW - up[0] * halfH;
        q.v[2].y = center[1] - right[1] * halfW - up[1] * halfH;
        q.v[2].z = center[2] - right[2] * halfW - up[2] * halfH;

        q.v[3].x = center[0] + right[0] * halfW - up[0] * halfH;
        q.v[3].y = center[1] + right[1] * halfW - up[1] * halfH;
        q.v[3].z = center[2] + right[2] * halfW - up[2] * halfH;
    }
    // UV assignment
    q.v[0].u = uv.u0; q.v[0].v = uv.v0;
    q.v[1].u = uv.u1; q.v[1].v = uv.v0;
    q.v[2].u = uv.u0; q.v[2].v = uv.v1;
    q.v[3].u = uv.u1; q.v[3].v = uv.v1;

    for (int i = 0; i < 4; ++i) q.v[i].color = color;
    return q;
}

// ---------------------------------------------------------------------------
// Weapon Diegetic Ammo Layout
// ---------------------------------------------------------------------------
// Generates all 3D quads in local weapon coordinate space:
// - Quad 0: Metallic outer bezel housing
// - Quad 1: Recessed dark screen glass
// - Quad 2..N: Emissive E99 phosphor digits and icons
inline int LayoutAmmoQuads(long gunVerts, int clip, int bin, Quad3D* outQuads, int maxQuads) {
    if (!outQuads || maxQuads < 10) return 0;
    const WeaponMountProfile prof = GetWeaponProfile(gunVerts);

    // Orientation basis with pitch tilt:
    // Forward = barrel axis (+Z), Right = lateral (+X), Up = vertical (+Y)
    const float rad = prof.pitchDeg * (3.14159265f / 180.0f);
    const float cp = std::cos(rad);
    const float sp = std::sin(rad);

    // Right vector remains along +X
    const float right[3] = { 1.0f, 0.0f, 0.0f };
    // Up vector rotated by pitch: (0, cp, sp)
    const float up[3] = { 0.0f, cp, sp };
    // Normal facing back towards shooter: (0, -sp, cp)
    const float norm[3] = { 0.0f, -sp, cp };

    const float center[3] = { prof.localX, prof.localY, prof.localZ };
    int count = 0;

    // 1. Outer Bezel Housing
    outQuads[count++] = BuildPlanarQuad(center, right, up, prof.widthUU * 0.5f, prof.heightUU * 0.5f,
                                        BezelBgRect(), ColorBezelMetal());

    // 2. Recessed Screen Face (slight push forward along normal)
    float screenCenter[3] = {
        center[0] + norm[0] * 0.015f,
        center[1] + norm[1] * 0.015f,
        center[2] + norm[2] * 0.015f
    };
    outQuads[count++] = BuildPlanarQuad(screenCenter, right, up, prof.widthUU * 0.46f, prof.heightUU * 0.44f,
                                        ScreenBgRect(), ColorScreenGlass());

    // 3. Emissive Phosphor Digits & Icons
    // Slight additional offset so digits float just above the screen glass
    float digitCenter[3] = {
        screenCenter[0] + norm[0] * 0.010f,
        screenCenter[1] + norm[1] * 0.010f,
        screenCenter[2] + norm[2] * 0.010f
    };

    const uint32_t ammoCol = AmmoColor(clip);
    const uint32_t binCol = ColorE99Cyan(180); // reserve slightly dimmer phosphor

    // Mini ammo icon on the far left
    float iconPos[3] = {
        digitCenter[0] - right[0] * (prof.widthUU * 0.35f),
        digitCenter[1] - right[1] * (prof.widthUU * 0.35f),
        digitCenter[2] - right[2] * (prof.widthUU * 0.35f)
    };
    outQuads[count++] = BuildPlanarQuad(iconPos, right, up, prof.heightUU * 0.28f, prof.heightUU * 0.28f,
                                        MagIconRect(), ammoCol);

    // Format clip digits (up to 2 digits: e.g. "08" or "30")
    int c = std::max(0, std::min(99, clip));
    int cTens = c / 10;
    int cOnes = c % 10;

    // Format bin digits (up to 3 digits: e.g. "120")
    int b = std::max(0, std::min(999, bin));
    int bHund = b / 100;
    int bTens = (b / 10) % 10;
    int bOnes = b % 10;

    const float digitW = prof.heightUU * 0.26f;
    const float digitH = prof.heightUU * 0.38f;

    // Positions across the screen width:
    // [Icon]  [ClipTens] [ClipOnes]  [/]  [BinHund] [BinTens] [BinOnes]
    // Center of screen is at 0; slash sits at X = 0.05
    float slashPos[3] = {
        digitCenter[0] + right[0] * 0.02f,
        digitCenter[1] + right[1] * 0.02f,
        digitCenter[2] + right[2] * 0.02f
    };
    outQuads[count++] = BuildPlanarQuad(slashPos, right, up, digitW * 0.40f, digitH * 0.45f,
                                        SlashRect(), ColorE99Cyan(160));

    // Clip Tens & Ones (to the left of slash)
    float cOnesPos[3] = {
        slashPos[0] - right[0] * (digitW * 0.95f),
        slashPos[1] - right[1] * (digitW * 0.95f),
        slashPos[2] - right[2] * (digitW * 0.95f)
    };
    outQuads[count++] = BuildPlanarQuad(cOnesPos, right, up, digitW * 0.5f, digitH * 0.5f,
                                        DigitRect(cOnes), ammoCol);

    if (cTens > 0 || c >= 10) {
        float cTensPos[3] = {
            cOnesPos[0] - right[0] * (digitW * 0.95f),
            cOnesPos[1] - right[1] * (digitW * 0.95f),
            cOnesPos[2] - right[2] * (digitW * 0.95f)
        };
        outQuads[count++] = BuildPlanarQuad(cTensPos, right, up, digitW * 0.5f, digitH * 0.5f,
                                            DigitRect(cTens), ammoCol);
    }

    // Bin Digits (to the right of slash, slightly smaller font)
    const float binW = digitW * 0.85f;
    const float binH = digitH * 0.85f;

    float bHundPos[3] = {
        slashPos[0] + right[0] * (binW * 0.95f),
        slashPos[1] + right[1] * (binW * 0.95f),
        slashPos[2] + right[2] * (binW * 0.95f)
    };
    float bTensPos[3] = {
        bHundPos[0] + right[0] * (binW * 0.95f),
        bHundPos[1] + right[1] * (binW * 0.95f),
        bHundPos[2] + right[2] * (binW * 0.95f)
    };
    float bOnesPos[3] = {
        bTensPos[0] + right[0] * (binW * 0.95f),
        bTensPos[1] + right[1] * (binW * 0.95f),
        bTensPos[2] + right[2] * (binW * 0.95f)
    };

    if (b >= 100) {
        outQuads[count++] = BuildPlanarQuad(bHundPos, right, up, binW * 0.5f, binH * 0.5f,
                                            DigitRect(bHund), binCol);
        outQuads[count++] = BuildPlanarQuad(bTensPos, right, up, binW * 0.5f, binH * 0.5f,
                                            DigitRect(bTens), binCol);
        outQuads[count++] = BuildPlanarQuad(bOnesPos, right, up, binW * 0.5f, binH * 0.5f,
                                            DigitRect(bOnes), binCol);
    } else if (b >= 10) {
        outQuads[count++] = BuildPlanarQuad(bHundPos, right, up, binW * 0.5f, binH * 0.5f,
                                            DigitRect(bTens), binCol);
        outQuads[count++] = BuildPlanarQuad(bTensPos, right, up, binW * 0.5f, binH * 0.5f,
                                            DigitRect(bOnes), binCol);
    } else {
        outQuads[count++] = BuildPlanarQuad(bHundPos, right, up, binW * 0.5f, binH * 0.5f,
                                            DigitRect(bOnes), binCol);
    }

    return count;
}

// ---------------------------------------------------------------------------
// Wrist Diegetic Health Gauge Layout
// ---------------------------------------------------------------------------
// Generates 3D quads in local left wrist / TMD coordinate space:
// - Quad 0: Metallic curved cuff bezel
// - Quad 1: Recessed dark gauge slot
// - Quad 2: Bio / cross emblem
// - Quad 3..12: 10 E99 phosphor segments (illuminated vs dim glass)
inline int LayoutHealthQuads(float hp, float maxHp, Quad3D* outQuads, int maxQuads) {
    if (!outQuads || maxQuads < 15) return 0;
    const WristMountProfile prof = GetWristProfile();

    float fraction = (maxHp > 0.0f) ? (hp / maxHp) : 0.0f;
    fraction = std::max(0.0f, std::min(1.0f, fraction));

    // Orientation basis with inward roll tilt:
    // Long = along forearm (+Z), Across = across wrist (+X), Normal = outward (+Y)
    const float rad = prof.rollDeg * (3.14159265f / 180.0f);
    const float cr = std::cos(rad);
    const float sr = std::sin(rad);

    // Vector along forearm (length axis): +Z
    const float fwd[3] = { 0.0f, 0.0f, 1.0f };
    // Across forearm with roll: (+cr, -sr, 0)
    const float across[3] = { cr, -sr, 0.0f };
    // Surface normal facing player's eyes: (+sr, +cr, 0)
    const float norm[3] = { sr, cr, 0.0f };

    const float center[3] = { prof.localX, prof.localY, prof.localZ };
    int count = 0;

    // 1. Outer Metallic Bezel Housing
    outQuads[count++] = BuildPlanarQuad(center, fwd, across, prof.lengthUU * 0.5f, prof.widthUU * 0.5f,
                                        BezelBgRect(), ColorBezelMetal());

    // 2. Recessed Dark Gauge Channel
    float gaugeCenter[3] = {
        center[0] + norm[0] * 0.015f,
        center[1] + norm[1] * 0.015f,
        center[2] + norm[2] * 0.015f
    };
    outQuads[count++] = BuildPlanarQuad(gaugeCenter, fwd, across, prof.lengthUU * 0.46f, prof.widthUU * 0.42f,
                                        ScreenBgRect(), ColorScreenGlass());

    // 3. Bio / Medical Emblem on the proximal end
    float iconCenter[3] = {
        gaugeCenter[0] - fwd[0] * (prof.lengthUU * 0.36f) + norm[0] * 0.010f,
        gaugeCenter[1] - fwd[1] * (prof.lengthUU * 0.36f) + norm[1] * 0.010f,
        gaugeCenter[2] - fwd[2] * (prof.lengthUU * 0.36f) + norm[2] * 0.010f
    };
    outQuads[count++] = BuildPlanarQuad(iconCenter, fwd, across, prof.widthUU * 0.28f, prof.widthUU * 0.28f,
                                        BioIconRect(), HealthColor(fraction));

    // 4. 10 Discrete E99 Phosphor Segments
    const int totalSegs = 10;
    const int filledSegs = (int)std::round(fraction * totalSegs);
    const uint32_t activeColor = HealthColor(fraction);
    const uint32_t emptyColor = ColorE99Cyan(40); // faint unlit glass trace

    // Segments span from proximal (after icon) to distal
    const float segStart = -prof.lengthUU * 0.22f;
    const float segEnd   = +prof.lengthUU * 0.42f;
    const float segSpan  = segEnd - segStart;
    const float segStep  = segSpan / (float)totalSegs;
    const float segHalfL = segStep * 0.40f;
    const float segHalfW = prof.widthUU * 0.30f;

    for (int i = 0; i < totalSegs; ++i) {
        const float segDist = segStart + ((float)i + 0.5f) * segStep;
        float segPos[3] = {
            gaugeCenter[0] + fwd[0] * segDist + norm[0] * 0.010f,
            gaugeCenter[1] + fwd[1] * segDist + norm[1] * 0.010f,
            gaugeCenter[2] + fwd[2] * segDist + norm[2] * 0.010f
        };
        const bool isFilled = (i < filledSegs);
        const AtlasRect uv = isFilled ? HealthSegFullRect() : HealthSegEmptyRect();
        const uint32_t color = isFilled ? activeColor : emptyColor;

        outQuads[count++] = BuildPlanarQuad(segPos, fwd, across, segHalfL, segHalfW, uv, color);
    }

    return count;
}

} // namespace diegetichud
