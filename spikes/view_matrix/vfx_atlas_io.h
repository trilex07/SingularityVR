#pragma once

// =============================================================================
// HOST-side atlas loading.
//
// Loads the four generated atlas DDS files (1024x1024 DXT5, full mip chain) with
// a small raw-DDS parser - no PNG decoder, no D3DX, no external dependency. If
// the files are missing it returns false and the caller falls back to
// host::BuildPlaceholderAtlas().
//
// The clip -> atlas/grid mapping is fixed and matches make_vfx_atlases.py and
// vfx_atlas_manifest.json (all atlases use a 4x4 grid).
// =============================================================================

#include "vfx_draw.h"

#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

namespace singularity_vfx {
namespace host {

inline bool ReadWholeFile(const char* path, std::vector<uint8_t>& out) {
    FILE* f = nullptr;
    if (fopen_s(&f, path, "rb") != 0 || !f) return false;
    fseek(f, 0, SEEK_END);
    const long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n <= 0) { fclose(f); return false; }
    out.resize(static_cast<size_t>(n));
    const size_t got = fread(out.data(), 1, out.size(), f);
    fclose(f);
    return got == out.size();
}

// Minimal DDS uploader for DXT1/DXT5 with an arbitrary mip count.
inline bool LoadDdsTexture(IDirect3DDevice9* dev, const uint8_t* b, size_t n,
                           IDirect3DTexture9** out) {
    if (out) *out = nullptr;
    if (!dev || !b || n < 128 || std::memcmp(b, "DDS ", 4) != 0) return false;

    const UINT height = *reinterpret_cast<const UINT*>(b + 12);
    const UINT width = *reinterpret_cast<const UINT*>(b + 16);
    UINT mips = *reinterpret_cast<const UINT*>(b + 28);
    const DWORD pfFlags = *reinterpret_cast<const DWORD*>(b + 80);
    const uint32_t fourcc = *reinterpret_cast<const uint32_t*>(b + 84);
    if (mips == 0) mips = 1;

    D3DFORMAT fmt;
    int blk;
    if ((pfFlags & 0x4) && fourcc == MAKEFOURCC('D', 'X', 'T', '1')) { fmt = D3DFMT_DXT1; blk = 8; }
    else if ((pfFlags & 0x4) && fourcc == MAKEFOURCC('D', 'X', 'T', '5')) { fmt = D3DFMT_DXT5; blk = 16; }
    else return false;

    if (FAILED(dev->CreateTexture(width, height, mips, 0, fmt, D3DPOOL_MANAGED, out, nullptr)) || !*out)
        return false;

    size_t off = 128;
    for (UINT i = 0; i < mips; ++i) {
        const UINT mw = (width >> i) ? (width >> i) : 1;
        const UINT mh = (height >> i) ? (height >> i) : 1;
        const size_t rowbytes = static_cast<size_t>((mw + 3) / 4) * blk;
        const UINT rows = (mh + 3) / 4;
        const size_t need = rowbytes * rows;
        if (off + need > n) { (*out)->Release(); *out = nullptr; return false; }

        D3DLOCKED_RECT lr;
        if (FAILED((*out)->LockRect(i, &lr, nullptr, 0))) { (*out)->Release(); *out = nullptr; return false; }
        for (UINT y = 0; y < rows; ++y)
            std::memcpy(static_cast<uint8_t*>(lr.pBits) + static_cast<size_t>(y) * lr.Pitch,
                        b + off + static_cast<size_t>(y) * rowbytes, rowbytes);
        (*out)->UnlockRect(i);
        off += need;
    }
    return true;
}

inline bool LoadAtlasTexture(IDirect3DDevice9* dev, const char* dir, const char* file,
                             IDirect3DTexture9** out) {
    char path[MAX_PATH];
    _snprintf_s(path, _TRUNCATE, "%s\\%s", dir, file);
    std::vector<uint8_t> bytes;
    if (!ReadWholeFile(path, bytes)) return false;
    return LoadDdsTexture(dev, bytes.data(), bytes.size(), out);
}

// Fills `atlas` from <dir>\vfx_{muzzle,smoke_white,smoke_black,blood}.dds.
// Returns false (and leaves atlas empty) if any atlas is missing/unsupported.
inline bool BuildAtlasFromDir(IDirect3DDevice9* dev, VfxAtlas& atlas, const char* dir) {
    if (!dev || !dir) return false;
    atlas = VfxAtlas{};
    for (auto& c : atlas.clips) { c.tex = -1; c.cols = 1; c.rows = 1; }

    if (!LoadAtlasTexture(dev, dir, "vfx_muzzle.dds", &atlas.texture[0]) ||
        !LoadAtlasTexture(dev, dir, "vfx_smoke_white.dds", &atlas.texture[1]) ||
        !LoadAtlasTexture(dev, dir, "vfx_smoke_black.dds", &atlas.texture[2]) ||
        !LoadAtlasTexture(dev, dir, "vfx_blood.dds", &atlas.texture[3])) {
        ReleaseAtlas(atlas);
        return false;
    }
    atlas.count = 4;

    const VfxAtlas::ClipMap muzzle{0, 4, 4};
    const VfxAtlas::ClipMap smokeW{1, 4, 4};
    const VfxAtlas::ClipMap smokeB{2, 4, 4};
    const VfxAtlas::ClipMap blood{3, 4, 4};
    atlas.clips[static_cast<int>(VfxClip::MuzzleFireSmall)]  = muzzle;
    atlas.clips[static_cast<int>(VfxClip::MuzzleFireLarge)]  = muzzle;
    atlas.clips[static_cast<int>(VfxClip::MuzzleFireHeavy)]  = muzzle;
    atlas.clips[static_cast<int>(VfxClip::MuzzleFireEnergy)] = muzzle;
    atlas.clips[static_cast<int>(VfxClip::SmokeWhite)]       = smokeW;
    atlas.clips[static_cast<int>(VfxClip::SmokeBlack)]       = smokeB;
    atlas.clips[static_cast<int>(VfxClip::BloodSplash)]      = blood;
    atlas.clips[static_cast<int>(VfxClip::BloodHitAlt)]      = blood;
    return true;
}

} // namespace host
} // namespace singularity_vfx
