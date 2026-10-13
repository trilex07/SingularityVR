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
                                              int halfX, int halfW, int dstH,
                                              int scalePct = 100) {
    CutsceneMonoFit out;
    if (srcW <= 0 || srcH <= 0 || halfW <= 0 || dstH <= 0 || halfX < 0)
        return out;
    // Fit scale preserving aspect; never upscale.
    double scale = (double)halfW / (double)srcW;
    const double scaleH = (double)dstH / (double)srcH;
    if (scaleH < scale) scale = scaleH;
    if (scale > 1.0) scale = 1.0;
    if (scalePct < 10) scalePct = 10;
    if (scalePct > 100) scalePct = 100;
    scale *= scalePct * 0.01;
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

// A camera-instance change is ambiguous: a Matinee camera CUT (same level,
// same pawn, engine still cinematic) must keep g_inCinematic, while a true
// level transition (pawn destroyed/recreated, or quit to a menu) must clear
// the stale latch. Proven by PID 20128: a helicopter-cut cleared CINE and the
// whole scene stayed stereo.
struct CameraChangeCtx {
    bool cineOn = false;
    bool pawnSame = false;   // g_playerPawn unchanged across the switch
    bool pawnValid = false;  // g_playerPawn != 0 now
    bool menuFocused = false;
    bool videoHandoff = false; // confirmed VideoFlat -> initial world load, native ON still present
};

inline bool CameraChangeClearsCine(CameraChangeCtx c) {
    if (!c.cineOn) return false;                    // nothing latched
    if (c.videoHandoff) return false;              // new pawn/loading menu is expected here
    if (!c.pawnValid || !c.pawnSame) return true;   // world went away
    if (c.menuFocused) return true;                 // quit to menu
    return false;                                   // Matinee cut: keep CINE
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

// ApplyVrFov computes a HALF-width eye projection while duplication is ON.
// CutsceneFlat executes one draw over the FULL-width source. Match that actual
// viewport or a square projects twice as wide. Submission retains eye tangents:
// the full mono source is subsequently fitted into one half for both eyes.
inline float CutsceneMonoRenderTanX(CutsceneMonoMode mode, bool flatEnabled,
                                    bool duplicated, float eyeTanX) {
    return mode == CutsceneMonoMode::CutsceneFlat && flatEnabled && duplicated
        ? eyeTanX * 2.f : eyeTanX;
}

inline bool CutsceneHideTrackedForeground(CutsceneMonoMode mode, bool flatEnabled,
    bool vrOn, bool menuFlat, bool sceneTarget, bool shadow, bool foreground,
    bool trackedMesh) {
    return CutsceneMonoShouldCompose(mode, flatEnabled, menuFlat, vrOn) &&
        sceneTarget && !shadow && foreground && trackedMesh;
}
