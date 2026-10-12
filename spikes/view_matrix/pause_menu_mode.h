#pragma once
// Live pause-menu presentation. Modes are different sources, not offset tweaks.
#include <cstdint>

namespace pausemode {
enum : int {
    Legacy = 0,       // one draw into the diegetic scene target
    HeadGfx = 1,      // redirect class-3 ortho batches into the 1280x720 pool
    FullGfxMono = 2,  // stretch the full scene colour target into that pool
    CanvasCapture = 3, // mirror PauseBackbufferCanvasRoute into full-size pool
    Count = 4
};

inline int Clamp(int mode) {
    if (mode < Legacy) return Legacy;
    if (mode >= Count) return Count - 1;
    return mode;
}

inline const char* ShortName(int mode) {
    switch (Clamp(mode)) {
        case HeadGfx: return "HEAD";
        case FullGfxMono: return "FULLGFX";
        case CanvasCapture: return "CANVAS";
        default: return "LEGACY";
    }
}

inline const char* SourceName(int mode) {
    switch (Clamp(mode)) {
        case HeadGfx: return "class3-ortho-pool-1280x720";
        case FullGfxMono: return "scene-colour-stretch";
        case CanvasCapture: return "backbuffer-canvas-pool";
        default: return "diegetic-scene";
    }
}

inline const char* PresentName(int mode) {
    switch (Clamp(mode)) {
        case HeadGfx: return "head-plane-if-captured";
        case FullGfxMono: return "head-plane-full-scene";
        case CanvasCapture: return "head-plane-full-canvas";
        default: return "native-stereo-halves";
    }
}

inline bool RedirectsOrtho(int mode) { return Clamp(mode) == HeadGfx; }
inline bool StretchesScene(int mode) { return Clamp(mode) == FullGfxMono; }
inline bool BakesBackbuffer(int mode) { (void)mode; return false; }
inline bool CapturesCanvasBackbuffer(int mode) { return Clamp(mode) == CanvasCapture; }
inline bool ShowsHeadCapture(int mode) {
    const int m = Clamp(mode);
    return m == HeadGfx || m == FullGfxMono || m == CanvasCapture;
}
}  // namespace pausemode
