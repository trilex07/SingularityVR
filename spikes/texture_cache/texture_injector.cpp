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
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>
#include "MinHook.h"
#include "resource_native_cache.h"
#include "texture_hook_timing.h"
#include "resource_positive_cache.h"
#include "remaster_scope.h"

// ---------------------------------------------------------------------------
// targets: FNV-1a 64 over the compact mip-0 DXT bytes of the original texture
// ---------------------------------------------------------------------------
struct Target {
    const char* name;
    UINT   w, h;
    D3DFORMAT fmt;
    uint64_t hash;       // FNV-1a 64 of original mip0
    const char* dds;     // replacement file inside TexturesDir
    const char* group;   // TextureGroups key: hands/weapons/enemies/env/glyphs/fx
};
#include "texture_groups.h"
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
static bool      g_resourceNativeCache = true;
static bool      g_bridgeOwned = false; // new route default OFF; never arms MinHook
static bool      g_remasterApply=false;
static remasterscope::PriorityMode g_priorityMaterialMode=remasterscope::PriorityMode::Off;
static remasterscope::PriorityMode g_valkyrieMaterialMode=remasterscope::PriorityMode::Off;
static remasterscope::PriorityMode g_centurionMaterialMode=remasterscope::PriorityMode::Off;
static remasterscope::PriorityMode g_kasimovMaterialMode=remasterscope::PriorityMode::Off;
static bool      g_effectsRemaster=false; // reviewed effects candidate, startup-only, default OFF
static char      g_remasterDir[MAX_PATH]="texture-override-remaster";
struct alignas(8) CacheCounters {
    volatile LONG64 binds=0, nativeHits=0, legacyHits=0, hashes=0;
    volatile LONG64 stored=0, storeFailed=0, evictions=0, candidateNotes=0;
};
static CacheCounters g_cacheCounters;
static textiming::Profiler g_textureTiming;
static ULONGLONG g_textureTimingStart=0;
static bool CandidateNote() {
    return InterlockedIncrement64(&g_cacheCounters.candidateNotes)<=64;
}
static char      g_texturesDir[MAX_PATH] = "texture-override";
static bool      g_groupOn[texgroups::GCount] = {true, true, true, true, true, true};
static bool      g_capture = false;           // dump locked mip0 of larger DXT textures
static char      g_capturesDir[MAX_PATH] = "capture";
static UINT      g_captureMin = 512;
static uint64_t  g_markerHash = 0;            // if set, only dump around this texture
static CRITICAL_SECTION g_logLock;
static FILE*     g_log = nullptr;

// capture state
static const int kCaptureMax = 400;
static uint64_t  g_captured[kCaptureMax];
static int       g_capturedCount = 0;
static IDirect3DTexture9* g_capSeen[kCaptureMax * 10];
static int       g_capSeenCount = 0;
static SRWLOCK   g_captureLock = SRWLOCK_INIT;

// marker mode: remember the last few hashed textures so that when the marker
// (the pistol's laser texture) is seen, the weapon textures bound just before it
// - which is what a material draw does - can be dumped and identified.
struct CaptureRing {
    UINT w = 0, h = 0;
    D3DFORMAT fmt = D3DFMT_UNKNOWN;
    uint64_t hash = 0;
    bool valid = false;
    std::vector<uint8_t> data;
};
static const int kRing = 24;
static CaptureRing g_ring[kRing];
static int  g_ringPos = 0;
static bool g_markerSeen = false;
static int  g_armed = 0;

static void Log(_Printf_format_string_ const char* fmt, ...) {
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
    g_resourceNativeCache=iniInt("general","resource_native_cache",1,ini)!=0;
    g_bridgeOwned=iniInt("general","bridge_owned",0,ini)!=0;
    g_remasterApply=iniInt("general","remaster_apply",0,ini)!=0;
    g_priorityMaterialMode=remasterscope::ParsePriorityMode(
        iniInt("general","priority_material_mode",0,ini));
    g_valkyrieMaterialMode=remasterscope::ParsePriorityMode(
        iniInt("general","valkyrie_material_mode",0,ini));
    g_centurionMaterialMode=remasterscope::ParsePriorityMode(
        iniInt("general","centurion_material_mode",0,ini));
    g_kasimovMaterialMode=remasterscope::ParsePriorityMode(
        iniInt("general","kasimov_material_mode",0,ini));
    g_effectsRemaster=iniInt("general","effects_remaster",0,ini)!=0;
    iniStr("general","remasterdir","texture-override-remaster",g_remasterDir,MAX_PATH,ini);
    iniStr("general", "texturesdir", "texture-override", g_texturesDir, MAX_PATH, ini);
    g_capture = iniInt("general", "capture", 0, ini) != 0;
    iniStr("general", "capturesdir", "capture", g_capturesDir, MAX_PATH, ini);
    g_captureMin = (UINT)iniInt("general", "capture_min", 512, ini);
    char mh[64] = "0";
    iniStr("general", "marker_hash", "0", mh, sizeof(mh), ini);
    g_markerHash = _strtoui64(mh, nullptr, 0);
    // Per-group switches ([TextureGroups], missing key = ON). Read once at
    // startup; the hub panel writes these keys (restart applies them).
    for (int g = 0; g < texgroups::GCount; ++g)
        g_groupOn[g] = iniInt("TextureGroups", texgroups::GroupName(g), 1, ini) != 0;
    // Quality tier: 2x (full set, in texturesdir) or 4K/8K proof sets in
    // texturesdir-4K/8K. Read once at startup; the hub writes the key and a
    // restart applies it. Missing tier files fall back to the original game
    // texture for that target (logged), never to another tier.
    char quality[16] = "2x";
    iniStr("TextureQuality", "Quality", "2x", quality, sizeof(quality), ini);
    {
        char suffixed[MAX_PATH];
        _snprintf_s(suffixed, _TRUNCATE, "%s%s", g_texturesDir,
                    texgroups::QualitySuffix(quality));
        strcpy_s(g_texturesDir, suffixed);
    }
    Log("config: enabled=%d apply=%d texturesDir=%s capture=%d min=%u marker=%016llX",
        g_enabled ? 1 : 0, g_apply ? 1 : 0, g_texturesDir, g_capture ? 1 : 0,
        g_captureMin, (unsigned long long)g_markerHash);
    Log("groups: hands=%d weapons=%d enemies=%d env=%d glyphs=%d fx=%d",
        g_groupOn[0] ? 1 : 0, g_groupOn[1] ? 1 : 0, g_groupOn[2] ? 1 : 0,
        g_groupOn[3] ? 1 : 0, g_groupOn[4] ? 1 : 0, g_groupOn[5] ? 1 : 0);
    Log("quality: %s (dir %s)", quality, g_texturesDir);
    Log("TEXTURE_NATIVE_CACHE enabled=%d version=1; target lookup/DDS pixels unchanged",
        g_resourceNativeCache?1:0);
    Log("priority material mode=%d (0=accepted base, 1=log only, 2=four-map remaster)",
        static_cast<int>(g_priorityMaterialMode));
    Log("weapon review modes: valkyrie=%d centurion=%d kasimov=%d (0=existing policy,1=log only,2=exact reviewed pair)",
        static_cast<int>(g_valkyrieMaterialMode),static_cast<int>(g_centurionMaterialMode),static_cast<int>(g_kasimovMaterialMode));
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
    if(g_bridgeOwned&&(img.w==0||img.h==0||img.w>8192||img.h>8192||mip>14))return false;
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
    if(g_bridgeOwned&&off!=(size_t)n)return false;
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
static std::size_t g_negativeCursor=0;
static SRWLOCK g_cacheLock = SRWLOCK_INIT;

static Entry* CacheFind(IDirect3DTexture9* t) {
    for (int i = 0; i < g_cacheCount; ++i) if (g_cache[i].tex == t) return &g_cache[i];
    return nullptr;
}
static Entry* CacheAdd(IDirect3DTexture9* t) {
    if (g_cacheCount >= kCacheMax) {
        if(g_resourceNativeCache) {
            const std::size_t slot=texnative::NegativeVictim(g_cache,kCacheMax,g_negativeCursor);
            if(slot==SIZE_MAX)return nullptr;
            g_cache[slot]=Entry();g_cache[slot].tex=t;
            InterlockedIncrement64(&g_cacheCounters.evictions);
            return &g_cache[slot];
        }
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

static void ReportCache() {
    AcquireSRWLockShared(&g_cacheLock);
    const int occupied=g_cacheCount;
    ReleaseSRWLockShared(&g_cacheLock);
    Log("TEXTURE_NATIVE_CACHE binds=%lld nativeHits=%lld legacyHits=%lld hashes=%lld stored=%lld storeFailed=%lld table=%d/512 evictions=%lld",
        InterlockedCompareExchange64(&g_cacheCounters.binds,0,0),
        InterlockedCompareExchange64(&g_cacheCounters.nativeHits,0,0),
        InterlockedCompareExchange64(&g_cacheCounters.legacyHits,0,0),
        InterlockedCompareExchange64(&g_cacheCounters.hashes,0,0),
        InterlockedCompareExchange64(&g_cacheCounters.stored,0,0),
        InterlockedCompareExchange64(&g_cacheCounters.storeFailed,0,0),occupied,
        InterlockedCompareExchange64(&g_cacheCounters.evictions,0,0));
    if constexpr(textiming::enabled) {
        for(unsigned n=0;n<static_cast<unsigned>(textiming::Route::count);++n) {
            const auto route=static_cast<textiming::Route>(n);
            const auto sample=g_textureTiming.Read(route);
            Log("TEXTURE_HOOK_CPU elapsed_ms=%llu route=%s samples=%lld classify_total_us=%.3f forward_total_us=%.3f classify_max_us=%.3f forward_max_us=%.3f; sparse CPU wall times NOT GPU time",
                (unsigned long long)(GetTickCount64()-g_textureTimingStart),textiming::Name(route),sample.samples,
                g_textureTiming.Us(sample.classifyTicks),g_textureTiming.Us(sample.forwardTicks),
                g_textureTiming.Us(sample.maxClassifyTicks),g_textureTiming.Us(sample.maxForwardTicks));
        }
    }
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
    IDirect3DDevice9Ex* ex=nullptr;
    const bool isEx=g_bridgeOwned&&SUCCEEDED(dev->QueryInterface(__uuidof(IDirect3DDevice9Ex),reinterpret_cast<void**>(&ex)));
    if(ex)ex->Release();
    IDirect3DTexture9* upload=nullptr;
    HRESULT hr = dev->CreateTexture(img.w, img.h, (UINT)img.levels, 0, img.fmt,
                                    isEx?D3DPOOL_DEFAULT:D3DPOOL_MANAGED, out, nullptr);
    if (FAILED(hr) || !*out) { Log("CreateTexture(%ux%u) failed 0x%08X", img.w, img.h, hr); return false; }
    if(isEx){
        hr=dev->CreateTexture(img.w,img.h,(UINT)img.levels,0,img.fmt,D3DPOOL_SYSTEMMEM,&upload,nullptr);
        if(FAILED(hr)||!upload){(*out)->Release();*out=nullptr;return false;}
    }
    IDirect3DTexture9* writer=upload?upload:*out;
    const int blk = (img.fmt == D3DFMT_DXT1) ? 8 : 16;
    for (int i = 0; i < img.levels; ++i) {
        D3DLOCKED_RECT lr;
        if (FAILED(writer->LockRect((UINT)i, &lr, nullptr, 0))) {
            Log("replacement LockRect level %d failed", i);
            if(upload)upload->Release();(*out)->Release(); *out = nullptr; return false;
        }
        UINT mw = (img.w >> i) ? (img.w >> i) : 1;
        UINT mh = (img.h >> i) ? (img.h >> i) : 1;
        size_t rowbytes = (size_t)((mw + 3) / 4) * blk;
        UINT rows = (mh + 3) / 4;
        const uint8_t* src = img.mip[i].data();
        for (UINT y = 0; y < rows; ++y)
            memcpy((uint8_t*)lr.pBits + (size_t)y * lr.Pitch, src + (size_t)y * rowbytes, rowbytes);
        hr=writer->UnlockRect((UINT)i);
        if(FAILED(hr)){if(upload)upload->Release();(*out)->Release();*out=nullptr;return false;}
    }
    if(upload){hr=dev->UpdateTexture(upload,*out);upload->Release();if(FAILED(hr)){(*out)->Release();*out=nullptr;return false;}}
    return true;
}

// ---------------------------------------------------------------------------
// runtime capture: dump the mip0 the engine actually uploads (the offline TFC
// path cannot reach some textures, e.g. the first-person pistol)
// ---------------------------------------------------------------------------
static bool WriteDdsMip0(const char* path, UINT w, UINT h, D3DFORMAT fmt,
                         const uint8_t* data, size_t size) {
    FILE* f = nullptr;
    if (fopen_s(&f, path, "wb") != 0 || !f) return false;
    uint8_t hdr[128] = {0};
    memcpy(hdr, "DDS ", 4);
    uint32_t dwSize = 124, flags = 0x1 | 0x2 | 0x4 | 0x1000 | 0x80000;
    uint32_t zero = 0, mips = 1, pfSize = 32, pfFlags = 0x4, caps = 0x1000;
    memcpy(hdr + 4, &dwSize, 4);
    memcpy(hdr + 8, &flags, 4);
    memcpy(hdr + 12, &h, 4);
    memcpy(hdr + 16, &w, 4);
    memcpy(hdr + 20, &zero, 4);
    memcpy(hdr + 24, &zero, 4);
    memcpy(hdr + 28, &mips, 4);
    memcpy(hdr + 76, &pfSize, 4);
    memcpy(hdr + 80, &pfFlags, 4);
    memcpy(hdr + 84, (fmt == D3DFMT_DXT1) ? "DXT1" : "DXT5", 4);
    memcpy(hdr + 108, &caps, 4);
    fwrite(hdr, 1, 128, f);
    fwrite(data, 1, size, f);
    fclose(f);
    return true;
}

static void DumpOne(UINT w, UINT h, D3DFORMAT fmt, uint64_t hv,
                    const uint8_t* data, size_t size, const char* tag) {
    char dir[MAX_PATH]; BuildPath(g_capturesDir, dir, sizeof(dir));
    CreateDirectoryA(dir, nullptr);
    const char* fc = (fmt == D3DFMT_DXT1) ? "dxt1" : "dxt5";
    char path[MAX_PATH];
    _snprintf_s(path, _TRUNCATE, "%s\\%s_%ux%u_%s_%016llX.dds", dir, tag, w, h, fc,
                (unsigned long long)hv);
    if (WriteDdsMip0(path, w, h, fmt, data, size))
        Log("DUMP %s %ux%u %s %016llX", tag, w, h, fc, (unsigned long long)hv);
}

static void MaybeCapture(IDirect3DTexture9* t) {
    if (!g_capture || !t) return;
    // Inspect each texture pointer exactly once. If the seen-set is full, STOP
    // capturing entirely - a fall-through previously re-locked every unseen
    // texture on every bind, which is what tanked the frame rate.
    AcquireSRWLockExclusive(&g_captureLock);
    for (int i = 0; i < g_capSeenCount; ++i)
        if (g_capSeen[i] == t) { ReleaseSRWLockExclusive(&g_captureLock); return; }
    if (g_capSeenCount >= (int)(sizeof(g_capSeen) / sizeof(g_capSeen[0]))) {
        g_capture = false;
        ReleaseSRWLockExclusive(&g_captureLock);
        Log("capture: seen-set full - capture disabled");
        return;
    }
    g_capSeen[g_capSeenCount++] = t;
    ReleaseSRWLockExclusive(&g_captureLock);

    D3DSURFACE_DESC d;
    if (FAILED(t->GetLevelDesc(0, &d))) return;
    if (d.Format != D3DFMT_DXT1 && d.Format != D3DFMT_DXT5) return;
    // Marker mode hashes everything down to 256 px so the laser marker is seen;
    // plain capture honours capture_min.
    const UINT floor = g_markerHash ? 256 : g_captureMin;
    if (d.Width < floor || d.Height < floor) return;
    D3DLOCKED_RECT lr;
    if (FAILED(t->LockRect(0, &lr, nullptr, D3DLOCK_READONLY))) return;
    const int blk = (d.Format == D3DFMT_DXT1) ? 8 : 16;
    const size_t rowbytes = (size_t)((d.Width + 3) / 4) * blk;
    const UINT rows = (d.Height + 3) / 4;
    std::vector<uint8_t> data(rowbytes * rows);
    for (UINT y = 0; y < rows; ++y)
        memcpy(data.data() + (size_t)y * rowbytes,
               (const uint8_t*)lr.pBits + (size_t)y * lr.Pitch, rowbytes);
    t->UnlockRect(0);
    const uint64_t hv = Fnv1a64(data.data(), data.size());

    bool dumpThis = false, dumpRing = false;
    AcquireSRWLockExclusive(&g_captureLock);
    if (g_markerHash) {
        CaptureRing& r = g_ring[g_ringPos];
        r.w = d.Width; r.h = d.Height; r.fmt = d.Format; r.hash = hv; r.valid = true;
        r.data.assign(data.begin(), data.end());
        g_ringPos = (g_ringPos + 1) % kRing;
        if (hv == g_markerHash) { g_markerSeen = true; g_armed = 48; dumpRing = true; }
        else if (g_armed > 0) { --g_armed; dumpThis = true; }
    } else {
        bool dup = false;
        for (int i = 0; i < g_capturedCount; ++i) if (g_captured[i] == hv) { dup = true; break; }
        if (!dup && g_capturedCount < kCaptureMax) { g_captured[g_capturedCount++] = hv; dumpThis = true; }
    }
    std::vector<CaptureRing> ringCopy;
    if (dumpRing) { for (int i = 0; i < kRing; ++i) ringCopy.push_back(g_ring[i]); }
    ReleaseSRWLockExclusive(&g_captureLock);

    if (dumpThis) DumpOne(d.Width, d.Height, d.Format, hv, data.data(), data.size(),
                          g_markerHash ? "pistol" : "cap");
    if (dumpRing) {
        Log("MARKER %016llX - dumping %d recent textures", (unsigned long long)hv, kRing);
        for (const CaptureRing& e : ringCopy)
            if (e.valid) DumpOne(e.w, e.h, e.fmt, e.hash, e.data.data(), e.data.size(), "pistol");
    }
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
            if (haveHash && t.hash == hash && remasterscope::ReviewTargetEnabled(t.name,g_kasimovMaterialMode)) return &t;
    }
    return nullptr;
}

static bool IsCandidate(const D3DSURFACE_DESC& d) {
    for (int i = 0; i < kTargetCount; ++i)
        if (kTargets[i].w == d.Width && kTargets[i].h == d.Height && kTargets[i].fmt == d.Format &&
            remasterscope::ReviewTargetEnabled(kTargets[i].name,g_kasimovMaterialMode))
            return true;
    return false;
}

static HRESULT STDMETHODCALLTYPE HookSetTexture(IDirect3DDevice9* dev, DWORD stage,
                                                IDirect3DBaseTexture9* tex) {
    if (!g_enabled || !tex || tex->GetType() != D3DRTYPE_TEXTURE)
        return RealSetTexture(dev, stage, tex);

    IDirect3DTexture9* t = static_cast<IDirect3DTexture9*>(tex);
    textiming::Scope timing(g_textureTiming);
    MaybeCapture(t);
    InterlockedIncrement64(&g_cacheCounters.binds);
    if(g_resourceNativeCache && texnative::KnownNative(t)) {
        InterlockedIncrement64(&g_cacheCounters.nativeHits);
        return timing.Finish(textiming::Route::native_hit,[&]{return RealSetTexture(dev,stage,tex);});
    }
    AcquireSRWLockShared(&g_cacheLock);
    Entry* e = CacheFind(t);
    if (e && e->replacement) {
        InterlockedIncrement64(&g_cacheCounters.legacyHits);
        IDirect3DTexture9* repl = e->replacement;
        ReleaseSRWLockShared(&g_cacheLock);
        return timing.Finish(textiming::Route::positive_hit,[&]{return RealSetTexture(dev,stage,repl);});
    }
    if (e && e->resolved) {
        InterlockedIncrement64(&g_cacheCounters.legacyHits);
        ReleaseSRWLockShared(&g_cacheLock);
        return timing.Finish(textiming::Route::legacy_native,[&]{return RealSetTexture(dev,stage,tex);});
    }
    ReleaseSRWLockShared(&g_cacheLock);

    D3DSURFACE_DESC desc;
    if (FAILED(t->GetLevelDesc(0, &desc)))
        return timing.Finish(textiming::Route::descriptor_error,[&]{return RealSetTexture(dev,stage,tex);});
    if (!IsCandidate(desc))
        return timing.Finish(textiming::Route::noncandidate,[&]{return RealSetTexture(dev,stage,tex);});

    InterlockedIncrement64(&g_cacheCounters.hashes);
    uint64_t hash = 0; bool have = HashLevel0(t, desc, hash);
    const Target* hit = MatchTarget(desc, hash, have);
    if(hit&&remasterscope::IsAddedBlood(hit->name)&&!g_effectsRemaster)hit=nullptr;
    if (hit && !texgroups::GroupAllows(g_groupOn,
            texgroups::GroupIndex(hit->group))) {
        Log("group off: %s (%s)", hit->name, hit->group);
        hit = nullptr;
    }

    // Existing resolved native decision, now owned by this resource rather
    // than competing with thousands of other negative pointer entries. A
    // destroyed texture automatically loses the stamp. Positive loading and
    // replacement bookkeeping below are deliberately unchanged.
    if(g_resourceNativeCache && !hit) {
        if(texnative::RememberNative(t)) {
            InterlockedIncrement64(&g_cacheCounters.stored);
            if(CandidateNote())Log("candidate %ux%u fmt=%d hash=%016llX native resource-cached haveHash=%d",
                desc.Width,desc.Height,desc.Format,(unsigned long long)hash,have?1:0);
            return timing.Finish(textiming::Route::hashed_native,[&]{return RealSetTexture(dev,stage,tex);});
        }
        InterlockedIncrement64(&g_cacheCounters.storeFailed);
        // Unsupported/OOM metadata: fall through to the original native
        // decision cache, with the bounded nonreplacement fallback above.
    }

    AcquireSRWLockExclusive(&g_cacheLock);
    e = CacheFind(t);
    if (!e) e = CacheAdd(t);
    if (e) {
        e->w = desc.Width; e->h = desc.Height;
        e->resolved = true; e->target = hit;
        if (hit && g_apply) {
            DDSImage img;
            char ddsPath[MAX_PATH];
            const bool effects= g_remasterApply && g_effectsRemaster && remasterscope::EffectsContains(hit->name);
            _snprintf_s(ddsPath, _TRUNCATE, "%s\\%s", effects?g_remasterDir:g_texturesDir, hit->dds);
            char full[MAX_PATH];
            if (ddsPath[0] == '\\' || (ddsPath[0] && ddsPath[1] == ':')) strcpy_s(full, ddsPath);
            else BuildPath(ddsPath, full, sizeof(full));
            if (LoadDDS(full, img) && img.fmt==desc.Format) {
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

    if(CandidateNote()) {
        if (!have) Log("candidate %ux%u fmt=%d: level0 not lockable (no hash)", desc.Width, desc.Height, desc.Format);
        else Log("candidate %ux%u fmt=%d hash=%016llX %s", desc.Width, desc.Height, desc.Format,
                 (unsigned long long)hash, hit ? hit->name : "(no match)");
    }

    return timing.Finish(repl?textiming::Route::hashed_positive:textiming::Route::hashed_native,
        [&]{return RealSetTexture(dev,stage,repl?repl:tex);});
}

// ---------------------------------------------------------------------------
// Opt-in callback: production owns slot65. No MinHook/vtable patch here.
extern "C" HRESULT WINAPI SingularityTextureBridgeV1(IDirect3DDevice9* dev,DWORD stage,
    IDirect3DBaseTexture9* texture,texbridge::Forward forward,unsigned abi,unsigned flags) {
    if(!forward)return E_POINTER;
    if(!g_bridgeOwned||abi!=texbridge::version||(flags&texbridge::protectedSource)||
       !g_enabled||!texture||texture->GetType()!=D3DRTYPE_TEXTURE)return forward(dev,stage,texture);
    auto* original=static_cast<IDirect3DTexture9*>(texture);
    InterlockedIncrement64(&g_cacheCounters.binds);
    if(g_resourceNativeCache&&texnative::KnownNative(original)){
        InterlockedIncrement64(&g_cacheCounters.nativeHits);return forward(dev,stage,texture);
    }
    if(auto* cached=texpositive::Acquire(original)){
        HRESULT hr=forward(dev,stage,cached);cached->Release();return hr;
    }
    D3DSURFACE_DESC desc{};
    if(FAILED(original->GetLevelDesc(0,&desc))||(desc.Usage&(D3DUSAGE_RENDERTARGET|D3DUSAGE_DEPTHSTENCIL|D3DUSAGE_DYNAMIC))||
       !IsCandidate(desc))return forward(dev,stage,texture);
    uint64_t hash=0;InterlockedIncrement64(&g_cacheCounters.hashes);
    bool have=HashLevel0(original,desc,hash);
    const Target* target=MatchTarget(desc,hash,have);
    if(target&&!texgroups::GroupAllows(g_groupOn,texgroups::GroupIndex(target->group)))target=nullptr;
    // New blood identities must not widen baseline replacement when OFF.
    if(target&&remasterscope::IsAddedBlood(target->name)&&!g_effectsRemaster)target=nullptr;
    const bool remaster=target&&remasterscope::Contains(target->name,g_effectsRemaster);
    if(target){
        static volatile LONG seen[kTargetCount]{};
        const unsigned slot=unsigned(target-kTargets);
        if(!InterlockedCompareExchange(&seen[slot],1,0))Log("BRIDGE MATCH %s exact original hash=%016llX remaster=%d remaster_apply=%d priority_mode=%d valkyrie_mode=%d centurion_mode=%d kasimov_mode=%d; named proof before substitution",
            target->name,(unsigned long long)hash,remaster?1:0,g_remasterApply?1:0,
            remasterscope::PriorityMaterial(target->name)?static_cast<int>(g_priorityMaterialMode):0,
            remasterscope::ValkyrieMaterial(target->name)?static_cast<int>(g_valkyrieMaterialMode):0,
            remasterscope::CenturionMaterial(target->name)?static_cast<int>(g_centurionMaterialMode):0,
            remasterscope::KasimovMaterial(target->name)?static_cast<int>(g_kasimovMaterialMode):0);
    }
    IDirect3DTexture9* replacement=nullptr;
    // LogOnly never selects the selected review group's remaster files. It may still select
    // the already accepted base override, unless global mode_apply=0.
    // Off retains the existing remaster_apply policy byte for byte.
    if(target&&g_apply){
        char relative[MAX_PATH],path[MAX_PATH];
        const bool useRemaster=remasterscope::UseMaterialRemaster(target->name,remaster,g_remasterApply,
            g_priorityMaterialMode,g_valkyrieMaterialMode,g_centurionMaterialMode,g_kasimovMaterialMode);
        _snprintf_s(relative,_TRUNCATE,"%s\\%s",useRemaster?g_remasterDir:g_texturesDir,target->dds);
        if(relative[0]=='\\'||relative[1]==':')strcpy_s(path,relative);else BuildPath(relative,path,sizeof(path));
        DDSImage image;
        if(LoadDDS(path,image)&&image.fmt==desc.Format&&CreateReplacement(dev,image,&replacement)){
            if(!texpositive::Store(original,replacement)){replacement->Release();replacement=nullptr;}
            else Log("BRIDGE SUBSTITUTE %s (%ux%u -> %ux%u) stage=%u remaster=%d; resource-owned",target->name,desc.Width,desc.Height,image.w,image.h,stage,useRemaster?1:0);
        }
    }
    if(!replacement&&g_resourceNativeCache)texnative::RememberNative(original);
    if(CandidateNote())Log("BRIDGE candidate %ux%u fmt=%d hash=%016llX name=%s lockable=%d apply=%d",
        desc.Width,desc.Height,desc.Format,(unsigned long long)hash,target?target->name:"native",have?1:0,g_apply?1:0);
    HRESULT hr=forward(dev,stage,replacement?replacement:texture);
    if(replacement)replacement->Release();return hr;
}

// DirectInput8Create forwarder
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
    g_textureTimingStart=GetTickCount64();
    g_textureTiming.Start();
    Log("TEXTURE_HOOK_CPU diagnostic=%d sparse=1/1024 cap=20000 deadline=300s; rendering/texture policy unchanged",textiming::enabled?1:0);
    // load the real (renamed) dinput8 so DirectInput8Create has somewhere to go
    LoadRealDinput8();
    Log("init: real dinput8 export=%p", RealDI8Create);

    if (!g_enabled) { Log("disabled by ini - injector idle"); return 0; }

    char renderIni[MAX_PATH];BuildPath("SingularityVR.ini",renderIni,sizeof(renderIni));
    const bool renderBridge=GetPrivateProfileIntA("Render","TextureBridgeOwned",0,renderIni)!=0;
    if(renderBridge!=g_bridgeOwned){Log("BRIDGE config mismatch; fail closed, no D3D hooks/substitution");return 0;}

    if(g_bridgeOwned){
        Log("BRIDGE READY ABI=1; production slot65 owner; no D3D9/CreateDevice/MinHook hooks armed");
        for(unsigned note=0;note<60;++note){Sleep(5000);ReportCache();}
        g_textureTiming.Stop();return 0;
    }

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
    // Existing initialization worker, no extra render/Present hooks. At most
    // 60 aggregate reports over five minutes; no per-bind filesystem work.
    for(unsigned note=0;note<60;++note){Sleep(5000);ReportCache();}
    g_textureTiming.Stop();
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

