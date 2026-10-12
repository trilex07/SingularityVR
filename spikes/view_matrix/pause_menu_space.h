#pragma once
// Pause GFx text is a 2D canvas orthographic matrix in full-frame clip space.
// StereoPair remaps the camera slots and scissors each eye, so that matrix is
// left covering the whole side-by-side buffer. Text on the left of the canvas
// survives only in the left eye and is cut by the seam. VR Settings does not
// use this matrix: it is a separate overlay projected into each eye.
//
// A canvas ortho has a pixel/twip scale on the diagonal and a (0,0,0,1) w
// column. The HUD eye remap then scales about the origin by the class-3 inset
// and applies x' = 0.5x ± 0.5, which is the same per-eye panel the inset
// already uses. This header only recognises that matrix and states the clip
// composition. It does not move the VR Settings row.

#include <cmath>

namespace pausemenuspace {

inline bool IsPauseCanvasOrtho(const float m[16]) {
    const float sx = m[0];
    const float sy = m[5];
    if (std::fabs(m[15] - 1.0f) > 0.001f) return false;
    if (!(std::fabs(sx) > 1.0e-6f && std::fabs(sx) < 0.01f)) return false;
    if (!(std::fabs(sy) > 1.0e-6f && std::fabs(sy) < 0.01f)) return false;
    if (std::fabs(m[1]) > 1.0e-3f || std::fabs(m[4]) > 1.0e-3f) return false;
    if (std::fabs(m[3]) > 1.0e-3f || std::fabs(m[7]) > 1.0e-3f) return false;
    if (std::fabs(m[11]) > 1.0e-3f) return false;
    return true;
}

// Class-3 backbuffer composition already implemented by HudStereoPair:
// inset about the origin, then the half-frame eye shift. inset is the
// class-3 menuSafe scale (0.84 at MenuInsetPct 100).
inline void HudEyeClip(float clipX, float clipY, float inset, int eye, float& outX, float& outY) {
    const float s = eye ? 0.5f : -0.5f;
    outX = 0.5f * (inset * clipX) + s;
    outY = inset * clipY;
}

// GFx pause/main movies render a side-by-side canvas into a scene-sized RT.
// One full-width draw parks text on the left half of the texture; the in-world
// monitor then shows that clipped strip in both eyes.
inline bool DiegeticMenuCanvas(const float m[16], unsigned rtWidth, unsigned sceneWidth) {
    if (!IsPauseCanvasOrtho(m) || sceneWidth < 640) return false;
    return rtWidth >= sceneWidth;
}

// Canvas-ortho pause text on the backbuffer must use per-eye c5 inset/remap,
// not the generic menuSafe scale on a world-sized matrix.
inline bool PauseBackbufferCanvasRoute(unsigned menuClass, bool backbuffer, const float m[16]) {
    return menuClass == 3 && backbuffer && IsPauseCanvasOrtho(m);
}

}  // namespace pausemenuspace
