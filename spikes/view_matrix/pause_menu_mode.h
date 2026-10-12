#pragma once
// Live pause-menu presentation. Modes are different sources, not offset tweaks.
#include <cstdint>

namespace pausemode {
enum : int {
    Legacy = 0,       // one draw into the diegetic scene target
    HeadGfx = 1,      // redirect class-3 ortho batches into the 1280x720 pool
    FullGfxMono = 2,  // stretch the full scene colour target into that pool
    FullSource = 3,   // scale the whole backbuffer into both SBS halves
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
        case FullSource: return "FULLSRC";
        default: return "LEGACY";
    }
}

inline const char* SourceName(int mode) {
    switch (Clamp(mode)) {
        case HeadGfx: return "class3-ortho-pool-1280x720";
        case FullGfxMono: return "scene-colour-stretch";
        case FullSource: return "backbuffer-both-halves";
        default: return "diegetic-scene";
    }
}

inline const char* PresentName(int mode) {
    switch (Clamp(mode)) {
        case HeadGfx: return "head-plane-if-captured";
        case FullGfxMono: return "head-plane-full-scene";
        case FullSource: return "identical-sbs-halves";
        default: return "native-stereo-halves";
    }
}

inline bool RedirectsOrtho(int mode) { return Clamp(mode) == HeadGfx; }
inline bool StretchesScene(int mode) { return Clamp(mode) == FullGfxMono; }
inline bool BakesBackbuffer(int mode) { return Clamp(mode) == FullSource; }
inline bool ShowsHeadCapture(int mode) {
    const int m = Clamp(mode);
    return m == HeadGfx || m == FullGfxMono;
}
}  // namespace pausemode
