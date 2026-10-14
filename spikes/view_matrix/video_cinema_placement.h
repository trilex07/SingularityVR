#pragma once
// Relative to the accepted per-eye Bink placement, not full SBS clip width.
// Scaling X and Y equally preserves the authored aspect and UVs.
inline int ClampVideoCinemaScale(int percent) {
    return percent < 10 ? 10 : percent > 100 ? 100 : percent;
}
inline void PlaceVideoInEye(float& x, float& y, int eye, int percent) {
    const float scale = ClampVideoCinemaScale(percent) * 0.005f;
    x = x * scale + (eye == 0 ? -0.5f : 0.5f);
    y *= scale;
}
