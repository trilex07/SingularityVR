#pragma once

// =============================================================================
// HOST side of the VFX plugin split (Variant A).
//
// Loads SingularityVfx.dll (if present) from the game's Binaries directory and
// forwards the event/collect seams to it. It has no D3D9 dependency: the host's
// own code draws the returned DrawItems inside the existing per-eye path.
//
// Absence of the DLL is a clean kill-switch: every method becomes a no-op.
// =============================================================================

#include "vfx_plugin.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <cstdio>

namespace singularity_vfx {
namespace host {

using VfxLogFn = void (*)(const char* line);

inline VfxLogFn& LogSink() {
    static VfxLogFn sink = nullptr;
    return sink;
}

// The host may route this to its own Log(); default is the debugger.
inline void SetLog(VfxLogFn fn) { LogSink() = fn; }

inline void VfxHostLog(const char* line) {
    if (LogSink()) LogSink()(line);
    OutputDebugStringA("[vfx-host] ");
    OutputDebugStringA(line);
    OutputDebugStringA("\n");
}

inline bool GetExeDir(char* out, size_t n) {
    if (!out || n == 0) return false;
    DWORD len = GetModuleFileNameA(nullptr, out, (DWORD)n);
    if (len == 0 || len >= n) { out[0] = '\0'; return false; }
    char* slash = out + len;
    while (slash > out && slash[-1] != '\\' && slash[-1] != '/') --slash;
    *slash = '\0';
    return true;
}

// ---------------------------------------------------------------------------
// Plugin handle: load, negotiate ABI, forward seams.
// ---------------------------------------------------------------------------
class Plugin {
public:
    // Idempotent. Returns true only if a compatible plugin is live.
    bool Load() {
        if (loaded()) return true;

        char dir[MAX_PATH] = {0};
        char path[MAX_PATH] = {0};
        if (GetExeDir(dir, sizeof(dir)))
            _snprintf_s(path, _TRUNCATE, "%s\\%s", dir, VFX_PLUGIN_FILENAME);

        module_ = path[0] ? LoadLibraryA(path) : nullptr;
        if (!module_) module_ = LoadLibraryA(VFX_PLUGIN_FILENAME);
        if (!module_) {
            VfxHostLog("plugin not present - VFX disabled");
            return false;
        }

        abiFn_ = reinterpret_cast<VfxAbiVersionFn>(GetProcAddress(module_, VFX_EXPORT_ABI));
        if (!abiFn_ || abiFn_() != VFX_PLUGIN_ABI_VERSION) {
            VfxHostLog("VFX ABI mismatch - refused");
            Unload();
            return false;
        }

        init_     = reinterpret_cast<VfxInitFn>(GetProcAddress(module_, VFX_EXPORT_INIT));
        shutdown_ = reinterpret_cast<VfxShutdownFn>(GetProcAddress(module_, VFX_EXPORT_SHUTDOWN));
        onFire_   = reinterpret_cast<VfxOnFireFn>(GetProcAddress(module_, VFX_EXPORT_FIRE));
        onHit_    = reinterpret_cast<VfxOnHitFn>(GetProcAddress(module_, VFX_EXPORT_HIT));
        tick_     = reinterpret_cast<VfxTickFn>(GetProcAddress(module_, VFX_EXPORT_TICK));
        collect_  = reinterpret_cast<VfxCollectFn>(GetProcAddress(module_, VFX_EXPORT_COLLECT));

        if (!onFire_ || !onHit_ || !tick_ || !collect_) {
            VfxHostLog("VFX plugin missing required exports - refused");
            Unload();
            return false;
        }
        if (init_ && !init_()) {
            VfxHostLog("VFX plugin init failed - refused");
            Unload();
            return false;
        }
        VfxHostLog("VFX plugin loaded (ABI 1)");
        return true;
    }

    void Unload() {
        if (shutdown_) shutdown_();
        if (module_) FreeLibrary(module_);
        module_ = nullptr; abiFn_ = nullptr; init_ = nullptr; shutdown_ = nullptr;
        onFire_ = nullptr; onHit_ = nullptr; tick_ = nullptr; collect_ = nullptr;
    }

    bool loaded() const { return module_ != nullptr; }

    // ---- seams (safe no-ops without a plugin) ----
    void OnFire(const VfxFireEvent& e) { if (onFire_) onFire_(&e); }
    void OnHit(const VfxHitEvent& e)   { if (onHit_)  onHit_(&e); }
    void Tick(float dt)                { if (tick_ && dt > 0.0f) tick_(dt); }
    int  Collect(VfxRenderPass pass, VfxDrawItem* out, int capacity) {
        if (!collect_) return 0;
        return collect_(static_cast<uint8_t>(pass), out, capacity);
    }

private:
    HMODULE         module_   = nullptr;
    VfxAbiVersionFn abiFn_    = nullptr;
    VfxInitFn       init_     = nullptr;
    VfxShutdownFn   shutdown_ = nullptr;
    VfxOnFireFn     onFire_   = nullptr;
    VfxOnHitFn      onHit_    = nullptr;
    VfxTickFn       tick_     = nullptr;
    VfxCollectFn    collect_  = nullptr;
};

} // namespace host
} // namespace singularity_vfx
