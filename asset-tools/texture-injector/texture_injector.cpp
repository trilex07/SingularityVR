// Singularity VR - standalone first-person texture override injector (research).
//
// Deployed as dinput8.dll (the game statically imports DINPUT8!DirectInput8Create
// and no dinput8.dll ships in Binaries, so this proxy loads at process start and
// CO-EXISTS with the VR mod's d3d9.dll). It intercepts IDirect3DDevice9::SetTexture
// and, for textures whose mip-0 DXT bytes hash to a known first-person target,
// binds a pre-upscaled 2x DDS instead. Nothing on disk in the game is modified
// apart from this DLL + its data folder, and deleting them restores vanilla.
//
// Deliberately minimal:
//   * no d3d9.cpp / VR runtime changes
//   * no package / TFC / executable changes
//   * ini-gated, log-only mode available, safe to disable
//
// See README.md. Research only - not part of the production mod.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d9.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>
#include "MinHook.h"

// ---------------------------------------------------------------------------
// targets: FNV-1a 64 over the compact mip-0 DXT bytes of the original texture
// ---------------------------------------------------------------------------
struct Target {
    const char* name;
    UINT   w, h;
    D3DFORMAT fmt;
    uint64_t hash;       // FNV-1a 64 of original mip0
    const char* dds;     // replacement file inside TexturesDir
};
// Target table is generated from the pipeline manifests (content hashes of the
// original mip-0 DXT bytes). Replacement filenames are variant-agnostic:
// build.ps1 -Variant A|B copies the chosen candidate into <name>.dds.
#include "targets_generated.h"

// ---------------------------------------------------------------------------
// state
// ---------------------------------------------------------------------------
static HMODULE   g_self = nullptr;
static bool      g_enabled = true;
static bool      g_apply = true;              // false = log only
static char      g_texturesDir[MAX_PATH] = "texture-override";
static CRITICAL_SECTION g_logLock;
static FILE*     g_log = nullptr;

static void Log(const char* fmt, ...) {
    if (!g_log) return;
    EnterCriticalSection(&g_logLock);
    va_list a; va_start(a, fmt);
    vfprintf(g_log, fmt, a);
    va_end(a);
    fputc('\n', g_log);
    fflush(g_log);
    LeaveCriticalSection(&g_logLock);
}

static void GetSelfDir(char* out, size_t n) {
    GetModuleFileNameA(g_self, out, (DWORD)n);
    char* s = strrchr(out, '\\'); if (s) *s = 0;
}

static void BuildPath(const char* leaf, char* out, size_t n) {
    char dir[MAX_PATH]; GetSelfDir(dir, sizeof(dir));
    _snprintf_s(out, n, _TRUNCATE, "%s\\%s", dir, leaf);
}

// ---------------------------------------------------------------------------
// ini
// ---------------------------------------------------------------------------
static int iniInt(const char* section, const char* key, int def, const char* path) {
    return (int)GetPrivateProfileIntA(section, key, def, path);
}
static void iniStr(const char* section, const char* key, const char* def,
                   char* out, DWORD n, const char* path) {
    GetPrivateProfileStringA(section, key, def, out, n, path);
}

static void LoadConfig() {
    char ini[MAX_PATH]; BuildPath("texture_override.ini", ini, sizeof(ini));
    g_enabled = iniInt("general", "enabled", 1, ini) != 0;
    g_apply   = iniInt("general", "mode_apply", 1, ini) != 0;
    iniStr("general", "texturesdir", "texture-override", g_texturesDir, MAX_PATH, ini);
    Log("config: enabled=%d apply=%d texturesDir=%s", g_enabled ? 1 : 0,
        g_apply ? 1 : 0, g_texturesDir);
}

// ---------------------------------------------------------------------------
// DDS
// ---------------------------------------------------------------------------
struct DDSImage {
    UINT w = 0, h = 0;
    D3DFORMAT fmt = D3DFMT_UNKNOWN;
    int levels = 0;
    std::vector<std::vector<uint8_t>> mip;
};

static bool LoadDDS(const char* path, DDSImage& img) {
    FILE* f = nullptr;
    if (fopen_s(&f, path, "rb") != 0 || !f) { Log("LoadDDS: cannot open %s", path); return false; }
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    if (n < 128) { fclose(f); return false; }
    std::vector<uint8_t> buf((size_t)n);
    if (fread(buf.data(), 1, (size_t)n, f) != (size_t)n) { fclose(f); return false; }
    fclose(f);
    if (memcmp(buf.data(), "DDS ", 4) != 0) { Log("LoadDDS: bad magic %s", path); return false; }
    img.h = *(const UINT*)&buf[12];
    img.w = *(const UINT*)&buf[16];
    UINT mip = *(const UINT*)&buf[28];
    uint32_t fourcc = *(const uint32_t*)&buf[84];
    int blk;
    if (fourcc == MAKEFOURCC('D', 'X', 'T', '1')) { img.fmt = D3DFMT_DXT1; blk = 8; }
    else if (fourcc == MAKEFOURCC('D', 'X', 'T', '5')) { img.fmt = D3DFMT_DXT5; blk = 16; }
    else { Log("LoadDDS: unsupported fourcc for %s", path); return false; }
    if (mip == 0) mip = 1;
    img.levels = (int)mip;
    size_t off = 128;
    for (UINT i = 0; i < mip; ++i) {
        UINT mw = (img.w >> i) ? (img.w >> i) : 1;
        UINT mh = (img.h >> i) ? (img.h >> i) : 1;
        size_t sz = (size_t)((mw + 3) / 4) * ((mh + 3) / 4) * blk;
        if (off + sz > (size_t)n) { Log("LoadDDS: truncated %s level %u", path, i); return false; }
        img.mip.emplace_back(buf.begin() + off, buf.begin() + off + sz);
        off += sz;
    }
    return true;
}

// ---------------------------------------------------------------------------
// classification cache
// ---------------------------------------------------------------------------
struct Entry {
    IDirect3DTexture9* tex = nullptr;
    UINT w = 0, h = 0;
    bool resolved = false;
    IDirect3DTexture9* replacement = nullptr;   // AddRef'd
    const Target* target = nullptr;
};
static const int kCacheMax = 512;
static Entry g_cache[kCacheMax];
static int   g_cacheCount = 0;
static SRWLOCK g_cacheLock = SRWLOCK_INIT;

static Entry* CacheFind(IDirect3DTexture9* t) {
    for (int i = 0; i < g_cacheCount; ++i) if (g_cache[i].tex == t) return &g_cache[i];
    return nullptr;
}
static Entry* CacheAdd(IDirect3DTexture9* t) {
    if (g_cacheCount >= kCacheMax) {
        // simple: drop the oldest non-replacement entry
        for (int i = 0; i < g_cacheCount; ++i) {
            if (!g_cache[i].replacement) { g_cache[i] = Entry(); g_cache[i].tex = t; return &g_cache[i]; }
        }
        return nullptr;
    }
    Entry* e = &g_cache[g_cacheCount++];
    e->tex = t;
    return e;
}

// ---------------------------------------------------------------------------
// hashing / substitution
// ---------------------------------------------------------------------------
static uint64_t Fnv1a64(const uint8_t* p, size_t n) {
    uint64_t h = 0xcbf29ce484222325ULL;
    for (size_t i = 0; i < n; ++i) { h ^= p[i]; h *= 0x100000001b3ULL; }
    return h;
}

static bool HashLevel0(IDirect3DTexture9* t, D3DSURFACE_DESC& desc, uint64_t& out) {
    if (FAILED(t->GetLevelDesc(0, &desc))) return false;
    if (desc.Format != D3DFMT_DXT1 && desc.Format != D3DFMT_DXT5) return false;
    D3DLOCKED_RECT lr;
    if (FAILED(t->LockRect(0, &lr, nullptr, D3DLOCK_READONLY))) return false;
    const int blk = (desc.Format == D3DFMT_DXT1) ? 8 : 16;
    const size_t rowbytes = (size_t)((desc.Width + 3) / 4) * blk;
    const UINT rows = (desc.Height + 3) / 4;
    uint64_t h = 0xcbf29ce484222325ULL;
    for (UINT y = 0; y < rows; ++y) {
        const uint8_t* p = (const uint8_t*)lr.pBits + (size_t)y * lr.Pitch;
        for (size_t x = 0; x < rowbytes; ++x) { h ^= p[x]; h *= 0x100000001b3ULL; }
    }
    t->UnlockRect(0);
    out = h;
    return true;
}

static bool CreateReplacement(IDirect3DDevice9* dev, const DDSImage& img,
                              IDirect3DTexture9** out) {
    *out = nullptr;
    HRESULT hr = dev->CreateTexture(img.w, img.h, (UINT)img.levels, 0, img.fmt,
                                    D3DPOOL_MANAGED, out, nullptr);
    if (FAILED(hr) || !*out) { Log("CreateTexture(%ux%u) failed 0x%08X", img.w, img.h, hr); return false; }
    const int blk = (img.fmt == D3DFMT_DXT1) ? 8 : 16;
    for (int i = 0; i < img.levels; ++i) {
        D3DLOCKED_RECT lr;
        if (FAILED((*out)->LockRect((UINT)i, &lr, nullptr, 0))) {
            Log("replacement LockRect level %d failed", i);
            (*out)->Release(); *out = nullptr; return false;
        }
        UINT mw = (img.w >> i) ? (img.w >> i) : 1;
        UINT mh = (img.h >> i) ? (img.h >> i) : 1;
        size_t rowbytes = (size_t)((mw + 3) / 4) * blk;
        UINT rows = (mh + 3) / 4;
        const uint8_t* src = img.mip[i].data();
        for (UINT y = 0; y < rows; ++y)
            memcpy((uint8_t*)lr.pBits + (size_t)y * lr.Pitch, src + (size_t)y * rowbytes, rowbytes);
        (*out)->UnlockRect((UINT)i);
    }
    return true;
}

// ---------------------------------------------------------------------------
// hooks
// ---------------------------------------------------------------------------
typedef IDirect3D9* (WINAPI* PFN_CREATE9)(UINT);
typedef HRESULT (WINAPI* PFN_CREATE9EX)(UINT, IDirect3D9Ex**);
typedef HRESULT (STDMETHODCALLTYPE* PFN_CREATEDEVICE)(IDirect3D9*, UINT, D3DDEVTYPE,
    HWND, DWORD, D3DPRESENT_PARAMETERS*, IDirect3DDevice9**);
typedef HRESULT (STDMETHODCALLTYPE* PFN_SETTEXTURE)(IDirect3DDevice9*, DWORD, IDirect3DBaseTexture9*);

static PFN_CREATE9      RealCreate9 = nullptr;
static PFN_CREATE9EX    RealCreate9Ex = nullptr;
static PFN_CREATEDEVICE RealCreateDevice = nullptr;
static PFN_SETTEXTURE   RealSetTexture = nullptr;
static bool             g_deviceHooked = false;
static bool             g_d3d9Hooked = false;

static HRESULT STDMETHODCALLTYPE HookSetTexture(IDirect3DDevice9* dev, DWORD stage,
                                                IDirect3DBaseTexture9* tex);

static void HookDevice(IDirect3DDevice9* dev) {
    if (g_deviceHooked || !dev) return;
    void** vt = *(void***)dev;
    if (MH_CreateHook(vt[65], &HookSetTexture, (void**)&RealSetTexture) != MH_OK) {
        Log("MH_CreateHook(SetTexture) failed");
        return;
    }
    MH_EnableHook(vt[65]);
    g_deviceHooked = true;
    Log("device hooked (SetTexture)");
}

static HRESULT STDMETHODCALLTYPE HookCreateDevice(IDirect3D9* self, UINT adapter, D3DDEVTYPE type,
        HWND focus, DWORD flags, D3DPRESENT_PARAMETERS* pp, IDirect3DDevice9** out) {
    HRESULT hr = RealCreateDevice(self, adapter, type, focus, flags, pp, out);
    if (SUCCEEDED(hr) && out && *out) HookDevice(*out);
    return hr;
}

static void HookD3D9(IDirect3D9* d3d) {
    if (!d3d || g_d3d9Hooked) return;
    void** vt = *(void***)d3d;
    if (MH_CreateHook(vt[16], &HookCreateDevice, (void**)&RealCreateDevice) == MH_OK) {
        MH_EnableHook(vt[16]);
        g_d3d9Hooked = true;
        Log("IDirect3D9 hooked (CreateDevice)");
    } else {
        Log("MH_CreateHook(CreateDevice) failed");
    }
}

static IDirect3D9* WINAPI HookCreate9(UINT sdk) {
    IDirect3D9* d3d = RealCreate9 ? RealCreate9(sdk) : nullptr;
    Log("Direct3DCreate9 called -> %p", d3d);
    HookD3D9(d3d);
    return d3d;
}
static HRESULT WINAPI HookCreate9Ex(UINT sdk, IDirect3D9Ex** out) {
    HRESULT hr = RealCreate9Ex ? RealCreate9Ex(sdk, out) : E_FAIL;
    Log("Direct3DCreate9Ex called -> 0x%08X", hr);
    if (SUCCEEDED(hr) && out && *out) HookD3D9((IDirect3D9*)*out);
    return hr;
}

static const Target* MatchTarget(const D3DSURFACE_DESC& d, uint64_t hash, bool& haveHash) {
    for (int i = 0; i < kTargetCount; ++i) {
        const Target& t = kTargets[i];
        if (t.w == d.Width && t.h == d.Height && t.fmt == d.Format)
            if (haveHash && t.hash == hash) return &t;
    }
    return nullptr;
}

static bool IsCandidate(const D3DSURFACE_DESC& d) {
    for (int i = 0; i < kTargetCount; ++i)
        if (kTargets[i].w == d.Width && kTargets[i].h == d.Height && kTargets[i].fmt == d.Format)
            return true;
    return false;
}

static HRESULT STDMETHODCALLTYPE HookSetTexture(IDirect3DDevice9* dev, DWORD stage,
                                                IDirect3DBaseTexture9* tex) {
    if (!g_enabled || !tex || tex->GetType() != D3DRTYPE_TEXTURE)
        return RealSetTexture(dev, stage, tex);

    IDirect3DTexture9* t = static_cast<IDirect3DTexture9*>(tex);
    AcquireSRWLockShared(&g_cacheLock);
    Entry* e = CacheFind(t);
    if (e && e->replacement) {
        IDirect3DTexture9* repl = e->replacement;
        ReleaseSRWLockShared(&g_cacheLock);
        return RealSetTexture(dev, stage, repl);
    }
    if (e && e->resolved) { ReleaseSRWLockShared(&g_cacheLock); return RealSetTexture(dev, stage, tex); }
    ReleaseSRWLockShared(&g_cacheLock);

    D3DSURFACE_DESC desc;
    if (FAILED(t->GetLevelDesc(0, &desc)) || !IsCandidate(desc))
        return RealSetTexture(dev, stage, tex);

    uint64_t hash = 0; bool have = HashLevel0(t, desc, hash);
    const Target* hit = MatchTarget(desc, hash, have);

    AcquireSRWLockExclusive(&g_cacheLock);
    e = CacheFind(t);
    if (!e) e = CacheAdd(t);
    if (e) {
        e->w = desc.Width; e->h = desc.Height;
        e->resolved = true; e->target = hit;
        if (hit && g_apply) {
            DDSImage img;
            char ddsPath[MAX_PATH];
            _snprintf_s(ddsPath, _TRUNCATE, "%s\\%s", g_texturesDir, hit->dds);
            char full[MAX_PATH];
            if (ddsPath[0] == '\\' || (ddsPath[0] && ddsPath[1] == ':')) strcpy_s(full, ddsPath);
            else BuildPath(ddsPath, full, sizeof(full));
            if (LoadDDS(full, img)) {
                IDirect3DTexture9* repl = nullptr;
                if (CreateReplacement(dev, img, &repl)) {
                    e->replacement = repl;   // kept for process lifetime
                    Log("SUBSTITUTE %s (%ux%u -> %ux%u) stage=%u", hit->name,
                        desc.Width, desc.Height, img.w, img.h, stage);
                }
            }
        }
    }
    IDirect3DTexture9* repl = e ? e->replacement : nullptr;
    ReleaseSRWLockExclusive(&g_cacheLock);

    if (!have) Log("candidate %ux%u fmt=%d: level0 not lockable (no hash)", desc.Width, desc.Height, desc.Format);
    else Log("candidate %ux%u fmt=%d hash=%016llX %s", desc.Width, desc.Height, desc.Format,
             (unsigned long long)hash, hit ? hit->name : "(no match)");

    return RealSetTexture(dev, stage, repl ? repl : tex);
}

// ---------------------------------------------------------------------------
// DirectInput8Create forwarder (our only required export)
// ---------------------------------------------------------------------------
typedef HRESULT (WINAPI* PFN_DI8CREATE)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
static PFN_DI8CREATE RealDI8Create = nullptr;

// The real dinput8 is loaded from a LOCAL COPY named dinput8_orig.dll (next to
// this DLL). Loading "dinput8.dll" by name would alias our own proxy module and
// recurse; the renamed copy is the standard proxy technique.
static void LoadRealDinput8() {
    if (RealDI8Create) return;
    char dir[MAX_PATH], orig[MAX_PATH];
    GetSelfDir(dir, sizeof(dir));
    _snprintf_s(orig, _TRUNCATE, "%s\\dinput8_orig.dll", dir);
    HMODULE real = LoadLibraryA(orig);
    if (!real) real = LoadLibraryA("dinput8_orig.dll");
    if (real) RealDI8Create = (PFN_DI8CREATE)GetProcAddress(real, "DirectInput8Create");
}

extern "C" HRESULT WINAPI DirectInput8Create(HINSTANCE hinst, DWORD version, REFIID riid,
                                             LPVOID* out, LPUNKNOWN outer) {
    LoadRealDinput8();
    if (!RealDI8Create) return E_FAIL;
    return RealDI8Create(hinst, version, riid, out, outer);
}

// ---------------------------------------------------------------------------
// init
// ---------------------------------------------------------------------------
static DWORD WINAPI InitThread(LPVOID) {
    // load the real (renamed) dinput8 so DirectInput8Create has somewhere to go
    LoadRealDinput8();
    Log("init: real dinput8 export=%p", RealDI8Create);

    if (!g_enabled) { Log("disabled by ini - injector idle"); return 0; }

    if (MH_Initialize() != MH_OK) { Log("MH_Initialize failed"); return 0; }

    HMODULE d3d9 = nullptr;
    for (int i = 0; i < 600 && !d3d9; ++i) { d3d9 = GetModuleHandleA("d3d9.dll"); if (!d3d9) Sleep(10); }
    if (!d3d9) { Log("d3d9.dll never loaded - giving up"); return 0; }
    Log("d3d9.dll = %p", d3d9);

    void* p9  = (void*)GetProcAddress(d3d9, "Direct3DCreate9");
    void* p9e = (void*)GetProcAddress(d3d9, "Direct3DCreate9Ex");
    if (p9 && MH_CreateHook(p9, &HookCreate9, (void**)&RealCreate9) == MH_OK) MH_EnableHook(p9);
    else Log("failed to hook Direct3DCreate9 (%p)", p9);
    if (p9e && MH_CreateHook(p9e, &HookCreate9Ex, (void**)&RealCreate9Ex) == MH_OK) MH_EnableHook(p9e);
    Log("hooks armed");
    return 0;
}

BOOL APIENTRY DllMain(HMODULE mod, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_self = mod;
        DisableThreadLibraryCalls(mod);
        InitializeCriticalSection(&g_logLock);
        char logPath[MAX_PATH]; BuildPath("texture_override.log", logPath, sizeof(logPath));
        fopen_s(&g_log, logPath, "w");
        Log("=== SingularityVR texture override injector (research) ===");
        LoadConfig();
        HANDLE h = CreateThread(nullptr, 0, InitThread, nullptr, 0, nullptr);
        if (h) CloseHandle(h);
    } else if (reason == DLL_PROCESS_DETACH) {
        if (g_log) { Log("detach"); fclose(g_log); g_log = nullptr; }
    }
    return TRUE;
}
