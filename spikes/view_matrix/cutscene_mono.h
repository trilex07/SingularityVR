#pragma once
#include <cstdint>

// CutsceneFlat mono-to-both-eyes composition math. Pure logic, no D3D
// dependency, so the offline unit test covers the exact geometry the
// Present-time code uses.
//
// Problem: in CutsceneFlat the engine draws the cinematic ONCE across the
// whole SBS backbuffer, but the XR bridge still submits one half per eye.
// Each eye then shows a different stretched half. The fix composes the full
// source image aspect-fit into the LEFT half and tells the bridge to submit
// that same half crop to both eyes (the validated menu crop coordinates).
struct CutsceneMonoRect { int left, top, right, bottom; }; // half-open

struct CutsceneMonoFit {
    bool valid = false;
    CutsceneMonoRect image{};          // aspect-fit destination inside the half
    CutsceneMonoRect bars[4]{};        // black regions inside the half
    int barCount = 0;
};

inline CutsceneMonoFit CutsceneMonoComputeFit(int srcW, int srcH,
                                              int halfX, int halfW, int dstH) {
    CutsceneMonoFit out;
    if (srcW <= 0 || srcH <= 0 || halfW <= 0 || dstH <= 0 || halfX < 0)
        return out;
    // Fit scale preserving aspect; never upscale.
    double scale = (double)halfW / (double)srcW;
    const double scaleH = (double)dstH / (double)srcH;
    if (scaleH < scale) scale = scaleH;
    if (scale > 1.0) scale = 1.0;
    if (scale <= 0.0) return out;
    const int dw = (int)(srcW * scale);
    const int dh = (int)(srcH * scale);
    if (dw <= 0 || dh <= 0 || dw > halfW || dh > dstH) return out;
    const int dx = halfX + (halfW - dw) / 2;
    const int dy = (dstH - dh) / 2;
    out.image = { dx, dy, dx + dw, dy + dh };
    // Bars, all clipped to the destination half [halfX, halfX+halfW) x [0, dstH).
    const int hx1 = halfX + halfW;
    if (dx > halfX)           out.bars[out.barCount++] = { halfX, 0, dx, dstH };
    if (dx + dw < hx1)        out.bars[out.barCount++] = { dx + dw, 0, hx1, dstH };
    if (dy > 0)               out.bars[out.barCount++] = { halfX, 0, hx1, dy };
    if (dy + dh < dstH)       out.bars[out.barCount++] = { halfX, dy + dh, hx1, dstH };
    out.valid = true;
    return out;
}

// Presentation predicate shared by the composer, the capture stamp and the
// cinema-matte skip so the three can never disagree about a frame.
enum class CutsceneMonoMode : std::uint8_t { Other, CutsceneFlat, VideoFlat };

inline bool CutsceneMonoShouldCompose(CutsceneMonoMode mode, bool flatEnabled,
                                      bool menuFlat, bool vrOn) {
    if (!vrOn || !flatEnabled || menuFlat) return false;
    // VideoFlat (Bink) already draws itself per eye via DrawVideoQuadPerEye;
    // composing it again would stack the two paths and double/crop the video.
    return mode == CutsceneMonoMode::CutsceneFlat;
}
