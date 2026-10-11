#pragma once
// GFx DIPUP positions are int16 pairs at the start of the vertex (MDG copies 8 bytes; the
// printed float is that bit pattern, not a clip-space xy). c5 is the row matrix:
//   clipX = vx * c[0] + vy * c[4] + c[12]
//   clipY = vx * c[1] + vy * c[5] + c[13]
// D3D9 then maps clip to the active viewport. Pause evidence (MDG 137, 5264x2684) and the
// main-menu canvas (MDG 74, 1280x720) do not share an aspect ratio, so scaling c5 by
// (poolW/sceneW, poolH/sceneH) stretches the panel. The pool mapping is a uniform contain
// of source viewport pixels into the target viewport.
#include <cmath>
#include <cstring>

namespace pausegfx {

constexpr float kFlashC8W = 1.f;
constexpr float kFlashDiagMin = 1e-7f;
constexpr float kFlashDiagMax = 1e-3f;

inline bool FlashMatrixShape(const float c[16]) {
    if (std::fabs(c[15] - kFlashC8W) > 0.001f) return false;
    if (std::fabs(c[0]) < kFlashDiagMin || std::fabs(c[0]) > kFlashDiagMax) return false;
    if (std::fabs(c[5]) < kFlashDiagMin || std::fabs(c[5]) > kFlashDiagMax) return false;
    if (std::fabs(c[10] - 1.f) > 0.001f) return false;
    return true;
}

struct FlashContainMap {
    float scale = 1.f;
    float ox = 0.f;
    float oy = 0.f;
    float alpha = 1.f;  // clipX scale after contain
    float beta = 1.f;   // clipY scale after contain
};

inline FlashContainMap MakeFlashContainMap(float sceneW, float sceneH, float poolW, float poolH) {
    FlashContainMap m{};
    if (sceneW <= 1.f || sceneH <= 1.f || poolW <= 1.f || poolH <= 1.f) return m;
    const float fitX = poolW / sceneW;
    const float fitY = poolH / sceneH;
    m.scale = fitX < fitY ? fitX : fitY;
    m.ox = (poolW - sceneW * m.scale) * 0.5f;
    m.oy = (poolH - sceneH * m.scale) * 0.5f;
    m.alpha = m.scale * sceneW / poolW;
    m.beta = m.scale * sceneH / poolH;
    return m;
}

inline void FlashInt16ToClip(const float c[16], int vx, int vy, float& clipX, float& clipY) {
    clipX = c[0] * (float)vx + c[4] * (float)vy + c[12];
    clipY = c[1] * (float)vx + c[5] * (float)vy + c[13];
}

// D3D9 viewport transform. vpX/vpY are the viewport origin in render-target pixels.
inline void ClipToViewportPixel(float clipX, float clipY, float vpX, float vpY, float vpW, float vpH,
                                float& px, float& py) {
    px = vpX + (clipX + 1.f) * vpW * 0.5f;
    py = vpY + (1.f - clipY) * vpH * 0.5f;
}

inline void ProjectFlashInt16ToPixel(const float c[16], int vx, int vy, float vpX, float vpY,
                                     float vpW, float vpH, float& px, float& py) {
    float clipX = 0.f, clipY = 0.f;
    FlashInt16ToClip(c, vx, vy, clipX, clipY);
    ClipToViewportPixel(clipX, clipY, vpX, vpY, vpW, vpH, px, py);
}

inline void MapSourcePixelContain(const FlashContainMap& m, float px, float py, float& outX, float& outY) {
    outX = px * m.scale + m.ox;
    outY = py * m.scale + m.oy;
}

// Closed form of contain: clip' = (alpha * clipX, beta * clipY) on the pool viewport.
inline void RemapFlashC5Contain(float c[16], float sceneW, float sceneH, float poolW, float poolH) {
    const FlashContainMap m = MakeFlashContainMap(sceneW, sceneH, poolW, poolH);
    c[0] *= m.alpha;
    c[4] *= m.alpha;
    c[8] *= m.alpha;
    c[12] *= m.alpha;
    c[1] *= m.beta;
    c[5] *= m.beta;
    c[9] *= m.beta;
    c[13] *= m.beta;
}

inline void RemapScissorContain(RECT& r, float sceneW, float sceneH, float poolW, float poolH) {
    const FlashContainMap m = MakeFlashContainMap(sceneW, sceneH, poolW, poolH);
    r.left = (LONG)lroundf((float)r.left * m.scale + m.ox);
    r.right = (LONG)lroundf((float)r.right * m.scale + m.ox);
    r.top = (LONG)lroundf((float)r.top * m.scale + m.oy);
    r.bottom = (LONG)lroundf((float)r.bottom * m.scale + m.oy);
}

// Logged c5 from the 2026-10-11 pause retest (%.4g). Vertices are recovered int16s, not NDC.
inline void MdgPause137FlashC5(float c[16]) {
    std::memset(c, 0, sizeof(c[0]) * 16);
    c[0] = 1.335e-5f;
    c[5] = -2.374e-5f;
    c[10] = 1.f;
    c[12] = -0.6404f;
    c[13] = 0.555f;
    c[15] = 1.f;
}

inline void MdgMain74FlashC5(float c[16]) {
    std::memset(c, 0, sizeof(c[0]) * 16);
    c[0] = 1.321e-5f;
    c[5] = -2.348e-5f;
    c[10] = 1.f;
    c[12] = -0.0003907f;
    c[13] = 0.414f;
    c[15] = 1.f;
}

}  // namespace pausegfx
