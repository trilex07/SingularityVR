#pragma once
// GFx flash batch c5-c8 convention (physical MDG, class 2 @ 1280x720, class 3 @ 5264x2684 SBS scene):
// ROW sum into clip: small |c[0]|, |c[5]| (~1.3e-5), c[10]=1, c[15]=1, translation in c[12],c[13].
// Diagonal scale is ~viewport-independent between main private RT and pause scene HDR; repositioning
// is mostly in translation (cockpit monitor clip vs centered 1280 canvas).
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

// Map a clip-space point through ROW c5-c8 (w=1 vertex).
inline void FlashRowClipXY(const float c[16], float vx, float vy, float& outX, float& outY) {
    outX = c[0] * vx + c[4] * vy + c[8] * 0.f + c[12];
    outY = c[1] * vx + c[5] * vy + c[9] * 0.f + c[13];
}

// Scene pause draws with full SBS colour viewport (e.g. 5264x2684). Pool is mono 1280x720 canvas.
// Remap: keep flash diagonal (already ~canvas-native), scale clip translation and row y/x coupling
// by pool/scene so the same GFx vertex lattice maps to the smaller RT (SBS width -> single panel).
inline void RemapFlashC5SceneSbsToPool(float c[16], float sceneW, float sceneH, float poolW,
                                      float poolH) {
    if (sceneW <= 1.f || sceneH <= 1.f || poolW <= 1.f || poolH <= 1.f) return;
    const float sx = poolW / sceneW;
    const float sy = poolH / sceneH;
    for (int row = 0; row < 4; ++row) {
        const int i = row * 4;
        c[i] *= sx;
        c[i + 1] *= sy;
    }
}

inline void RemapScissorSceneSbsToPool(RECT& r, float sceneW, float sceneH, float poolW,
                                       float poolH) {
    if (sceneW <= 1.f || sceneH <= 1.f) return;
    const float sx = poolW / sceneW;
    const float sy = poolH / sceneH;
    r.left = (LONG)lroundf((float)r.left * sx);
    r.right = (LONG)lroundf((float)r.right * sx);
    r.top = (LONG)lroundf((float)r.top * sy);
    r.bottom = (LONG)lroundf((float)r.bottom * sy);
}

// MDG reference matrices (physical 20261011, same process — for L1 regression only).
inline void MdgMainMenuFlashC5(float c[16]) {
    std::memset(c, 0, sizeof(c[0]) * 16);
    c[0] = 1.321e-5f;
    c[5] = -2.348e-5f;
    c[10] = 1.f;
    c[12] = -0.0003907f;
    c[13] = 0.414f;
    c[15] = 1.f;
}

inline void MdgPauseFlashC5(float c[16]) {
    std::memset(c, 0, sizeof(c[0]) * 16);
    c[0] = 1.335e-5f;
    c[5] = -2.374e-5f;
    c[10] = 1.f;
    c[12] = -0.6404f;
    c[13] = 0.555f;
    c[15] = 1.f;
}

}  // namespace pausegfx
