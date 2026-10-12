#pragma once
// Read-only pause GFx ownership probe. No engine memory writes.
#include <Windows.h>
#include <cstdint>

extern void Log(const char* fmt, ...);
extern bool IniPath(char* out);
extern UINT g_srcW;
extern UINT g_srcH;
extern volatile LONG g_vrMenuClass;

namespace pausegfxprobe {

inline bool Enabled() {
    char path[MAX_PATH]{};
    if (!IniPath(path)) return false;
    return GetPrivateProfileIntA("Diagnostic", "PauseGfxProbe", 0, path) != 0;
}

inline void LogPauseFocus(std::uintptr_t movie, void* viewport) {
    if (!Enabled() || !movie) return;
    static std::uintptr_t lastMovie = 0;
    if (lastMovie == movie) return;
    lastMovie = movie;

    int vpW = 0, vpH = 0;
    if (viewport) {
        vpW = *reinterpret_cast<int*>(reinterpret_cast<std::uintptr_t>(viewport) + 0x4Cu);
        vpH = *reinterpret_cast<int*>(reinterpret_cast<std::uintptr_t>(viewport) + 0x50u);
    }
    Log("pause-gfx-probe: focus movie=%p FViewport=%p SizeX=%d SizeY=%d backbuffer=%ux%u",
        (void*)movie, viewport, vpW, vpH, (unsigned)g_srcW, (unsigned)g_srcH);
    Log("pause-gfx-probe: expect class=3 RvUIGFxMovie_Pause; native render entry NOT hooked yet");
}

}  // namespace pausegfxprobe
