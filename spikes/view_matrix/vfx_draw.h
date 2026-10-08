#pragma once

// =============================================================================
// HOST-side D3D9 drawing for VFX DrawItems.
//
// IMPORTANT: this is called from the host's EXISTING per-eye draw path, once per
// eye, so the effect inherits the mod's stereo duplication. It is not a separate
// screen-space pass - drawing beside the per-eye path is what produces the SBS
// seam artefact.
//
// It saves and restores every D3D9 state it touches, draws camera-facing quads
// with additive (flash) or alpha (smoke/blood) blending, depth-tested but not
// depth-written, and batches by texture/blend.
// =============================================================================

#include "vfx_plugin.h"

#include <d3d9.h>
#include <cstddef>
#include <cmath>

namespace singularity_vfx {
namespace host {

struct VfxVertex {
    float    x, y, z;
    D3DCOLOR color;
    float    u, v;
};
static const DWORD kVfxFvf = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1;

// Atlas description: the HOST owns this and fills it from the clip manifest the
// plugin/atlas build produces. clipId indexes clips[].
struct VfxAtlas {
    static constexpr int kMaxTextures = 4;
    IDirect3DTexture9* texture[kMaxTextures] = {};
    int count = 0;

    struct ClipMap {
        int tex = -1;   // index into texture[]
        int cols = 1;
        int rows = 1;
    };
    ClipMap clips[static_cast<int>(VfxClip::Count)];

    const ClipMap* Lookup(uint16_t clipId) const {
        if (clipId >= static_cast<uint16_t>(VfxClip::Count)) return nullptr;
        return &clips[clipId];
    }
};

// ---------------------------------------------------------------------------
// Procedural placeholder atlas.
//
// Lets the host render testable flash/smoke/blood immediately, with NO external
// files, NO PNG decode and NO game assets. Replace with the real 1024x1024
// atlases from the manifest when they land; the clipId mapping is identical.
// ---------------------------------------------------------------------------
inline bool MakeBlobTexture(IDirect3DDevice9* dev, D3DCOLOR rgb, float alphaScale,
                            int cols, int rows, int cell, IDirect3DTexture9** out) {
    if (out) *out = nullptr;
    if (!dev || !out || cols <= 0 || rows <= 0 || cell <= 0) return false;
    const UINT size = static_cast<UINT>(cols * cell);
    if (FAILED(dev->CreateTexture(size, size, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
                                  out, nullptr)) || !*out)
        return false;

    D3DLOCKED_RECT lr;
    if (FAILED((*out)->LockRect(0, &lr, nullptr, 0))) {
        (*out)->Release(); *out = nullptr; return false;
    }
    const int total = cols * rows;
    const float inv = (cell > 1) ? 1.0f / static_cast<float>(cell - 1) : 0.0f;
    for (int y = 0; y < static_cast<int>(size); ++y) {
        DWORD* row = reinterpret_cast<DWORD*>(reinterpret_cast<BYTE*>(lr.pBits) +
                                              static_cast<size_t>(y) * lr.Pitch);
        const int cy = y / cell;
        for (int x = 0; x < static_cast<int>(size); ++x) {
            const int cx = x / cell;
            const int frame = cy * cols + cx;
            const float lx = static_cast<float>(x % cell) * inv * 2.0f - 1.0f;
            const float ly = static_cast<float>(y % cell) * inv * 2.0f - 1.0f;
            const float r = std::sqrt(lx * lx + ly * ly);
            const float t = (total > 1) ? static_cast<float>(frame) / (total - 1) : 0.0f;
            const float radius = 0.35f + 0.60f * t;
            float a = 1.0f - (r - radius * 0.35f) / (radius * 0.65f);
            if (a < 0.0f) a = 0.0f;
            if (a > 1.0f) a = 1.0f;
            a = a * a * (3.0f - 2.0f * a);            // smoothstep
            const BYTE A = static_cast<BYTE>(a * 255.0f * alphaScale);
            row[x] = (static_cast<DWORD>(A) << 24) | (rgb & 0x00FFFFFFu);
        }
    }
    (*out)->UnlockRect(0);
    return true;
}

// Builds 4 placeholder textures and fills the clip mapping. Returns false only if
// texture creation fails (caller can then fall back to no VFX).
inline void ReleaseAtlas(VfxAtlas& atlas);

inline bool BuildPlaceholderAtlas(IDirect3DDevice9* dev, VfxAtlas& atlas) {
    if (!dev) return false;
    atlas = VfxAtlas{};
    for (auto& c : atlas.clips) { c.tex = -1; c.cols = 1; c.rows = 1; }

    const int cell = 64;
    const bool ok = MakeBlobTexture(dev, D3DCOLOR_XRGB(255, 224, 150), 1.00f, 4, 4, cell, &atlas.texture[0]) &&
                    MakeBlobTexture(dev, D3DCOLOR_XRGB(210, 210, 210), 0.55f, 5, 5, cell, &atlas.texture[1]) &&
                    MakeBlobTexture(dev, D3DCOLOR_XRGB(38, 38, 38),    0.60f, 5, 5, cell, &atlas.texture[2]) &&
                    MakeBlobTexture(dev, D3DCOLOR_XRGB(150, 24, 24),   0.85f, 4, 4, cell, &atlas.texture[3]);
    if (!ok) { ReleaseAtlas(atlas); return false; }
    atlas.count = 4;

    const VfxAtlas::ClipMap muzzle{0, 4, 4};
    const VfxAtlas::ClipMap smokeW{1, 5, 5};
    const VfxAtlas::ClipMap smokeB{2, 5, 5};
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

inline void ReleaseAtlas(VfxAtlas& atlas) {
    for (int i = 0; i < VfxAtlas::kMaxTextures; ++i) {
        if (atlas.texture[i]) { atlas.texture[i]->Release(); atlas.texture[i] = nullptr; }
    }
    atlas.count = 0;
}

struct VfxSavedState {
    DWORD rsZEnable = 0, rsZWrite = 0, rsAlphaBlend = 0;
    DWORD rsSrcBlend = 0, rsDestBlend = 0, rsCull = 0;
    DWORD rsLighting = 0, rsFog = 0, rsAlphaTest = 0;
    DWORD colorOp[2] = {}, colorArg1[2] = {}, colorArg2[2] = {};
    DWORD alphaOp[2] = {}, alphaArg1[2] = {}, alphaArg2[2] = {};
    DWORD sampMin[2] = {}, sampMag[2] = {}, sampMip[2] = {}, sampU[2] = {}, sampV[2] = {};
    IDirect3DBaseTexture9* tex[2] = {};
    IDirect3DVertexShader9* vs = nullptr;
    IDirect3DPixelShader9* ps = nullptr;
    IDirect3DVertexDeclaration9* decl = nullptr;
    IDirect3DVertexBuffer9* stream = nullptr;
    UINT streamOffset = 0, streamStride = 0;
    DWORD fvf = 0;
    // Fixed-function transforms. The engine draws with shaders and leaves these
    // stale/identity, so untransformed XYZ quads would land nowhere without them.
    // Production divergence from the pattern worktree: the host passes the live
    // per-eye view*proj here (row-major D3DMATRIX).
    D3DMATRIX world, view, proj;
    bool haveTransforms = false;
};

inline void SaveVfxState(IDirect3DDevice9* dev, VfxSavedState& s) {
    dev->GetRenderState(D3DRS_ZENABLE, &s.rsZEnable);
    dev->GetRenderState(D3DRS_ZWRITEENABLE, &s.rsZWrite);
    dev->GetRenderState(D3DRS_ALPHABLENDENABLE, &s.rsAlphaBlend);
    dev->GetRenderState(D3DRS_SRCBLEND, &s.rsSrcBlend);
    dev->GetRenderState(D3DRS_DESTBLEND, &s.rsDestBlend);
    dev->GetRenderState(D3DRS_CULLMODE, &s.rsCull);
    dev->GetRenderState(D3DRS_LIGHTING, &s.rsLighting);
    dev->GetRenderState(D3DRS_FOGENABLE, &s.rsFog);
    dev->GetRenderState(D3DRS_ALPHATESTENABLE, &s.rsAlphaTest);
    for (DWORD st = 0; st < 2; ++st) {
        dev->GetTextureStageState(st, D3DTSS_COLOROP, &s.colorOp[st]);
        dev->GetTextureStageState(st, D3DTSS_COLORARG1, &s.colorArg1[st]);
        dev->GetTextureStageState(st, D3DTSS_COLORARG2, &s.colorArg2[st]);
        dev->GetTextureStageState(st, D3DTSS_ALPHAOP, &s.alphaOp[st]);
        dev->GetTextureStageState(st, D3DTSS_ALPHAARG1, &s.alphaArg1[st]);
        dev->GetTextureStageState(st, D3DTSS_ALPHAARG2, &s.alphaArg2[st]);
        dev->GetSamplerState(st, D3DSAMP_MINFILTER, &s.sampMin[st]);
        dev->GetSamplerState(st, D3DSAMP_MAGFILTER, &s.sampMag[st]);
        dev->GetSamplerState(st, D3DSAMP_MIPFILTER, &s.sampMip[st]);
        dev->GetSamplerState(st, D3DSAMP_ADDRESSU, &s.sampU[st]);
        dev->GetSamplerState(st, D3DSAMP_ADDRESSV, &s.sampV[st]);
        dev->GetTexture(st, &s.tex[st]);
    }
    dev->GetVertexShader(&s.vs);
    dev->GetPixelShader(&s.ps);
    dev->GetVertexDeclaration(&s.decl);
    dev->GetStreamSource(0, &s.stream, &s.streamOffset, &s.streamStride);
    dev->GetFVF(&s.fvf);
    s.haveTransforms = SUCCEEDED(dev->GetTransform(D3DTS_WORLD, &s.world)) &&
                       SUCCEEDED(dev->GetTransform(D3DTS_VIEW, &s.view)) &&
                       SUCCEEDED(dev->GetTransform(D3DTS_PROJECTION, &s.proj));
}

inline void RestoreVfxState(IDirect3DDevice9* dev, const VfxSavedState& s) {
    dev->SetRenderState(D3DRS_ZENABLE, s.rsZEnable);
    dev->SetRenderState(D3DRS_ZWRITEENABLE, s.rsZWrite);
    dev->SetRenderState(D3DRS_ALPHABLENDENABLE, s.rsAlphaBlend);
    dev->SetRenderState(D3DRS_SRCBLEND, s.rsSrcBlend);
    dev->SetRenderState(D3DRS_DESTBLEND, s.rsDestBlend);
    dev->SetRenderState(D3DRS_CULLMODE, s.rsCull);
    dev->SetRenderState(D3DRS_LIGHTING, s.rsLighting);
    dev->SetRenderState(D3DRS_FOGENABLE, s.rsFog);
    dev->SetRenderState(D3DRS_ALPHATESTENABLE, s.rsAlphaTest);
    for (DWORD st = 0; st < 2; ++st) {
        dev->SetTextureStageState(st, D3DTSS_COLOROP, s.colorOp[st]);
        dev->SetTextureStageState(st, D3DTSS_COLORARG1, s.colorArg1[st]);
        dev->SetTextureStageState(st, D3DTSS_COLORARG2, s.colorArg2[st]);
        dev->SetTextureStageState(st, D3DTSS_ALPHAOP, s.alphaOp[st]);
        dev->SetTextureStageState(st, D3DTSS_ALPHAARG1, s.alphaArg1[st]);
        dev->SetTextureStageState(st, D3DTSS_ALPHAARG2, s.alphaArg2[st]);
        dev->SetSamplerState(st, D3DSAMP_MINFILTER, s.sampMin[st]);
        dev->SetSamplerState(st, D3DSAMP_MAGFILTER, s.sampMag[st]);
        dev->SetSamplerState(st, D3DSAMP_MIPFILTER, s.sampMip[st]);
        dev->SetSamplerState(st, D3DSAMP_ADDRESSU, s.sampU[st]);
        dev->SetSamplerState(st, D3DSAMP_ADDRESSV, s.sampV[st]);
        dev->SetTexture(st, s.tex[st]);
    }
    dev->SetVertexShader(s.vs);
    dev->SetPixelShader(s.ps);
    dev->SetVertexDeclaration(s.decl);
    dev->SetStreamSource(0, s.stream, s.streamOffset, s.streamStride);
    dev->SetFVF(s.fvf);
    if (s.haveTransforms) {
        dev->SetTransform(D3DTS_WORLD, &s.world);
        dev->SetTransform(D3DTS_VIEW, &s.view);
        dev->SetTransform(D3DTS_PROJECTION, &s.proj);
    }
    if (s.tex[0]) s.tex[0]->Release();
    if (s.tex[1]) s.tex[1]->Release();
    if (s.vs) s.vs->Release();
    if (s.ps) s.ps->Release();
    if (s.decl) s.decl->Release();
    if (s.stream) s.stream->Release();
}

// Draws a list of DrawItems as world-space camera-facing quads. Must be called
// inside each eye's world draw. `camRight`/`camUp` are the world-space camera
// basis from that eye's view matrix.
class VfxDrawer {
public:
    static constexpr int kMaxBatchVerts = 6 * 64;   // 64 quads per flush

    bool Init(IDirect3DDevice9* dev) {
        if (vb_ || !dev) return vb_ != nullptr;
        const UINT bytes = sizeof(VfxVertex) * kMaxBatchVerts;
        if (FAILED(dev->CreateVertexBuffer(bytes, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
                                           kVfxFvf, D3DPOOL_DEFAULT, &vb_, nullptr))) {
            vb_ = nullptr;
            return false;
        }
        return true;
    }

    void Shutdown() {
        if (vb_) { vb_->Release(); vb_ = nullptr; }
    }

    void DrawItems(IDirect3DDevice9* dev, const VfxDrawItem* items, int count,
                   const VfxAtlas& atlas, const float camRight[3], const float camUp[3],
                   const float eyeProj[16]) {
        if (!dev || !items || count <= 0 || !vb_ || !eyeProj) return;

        VfxSavedState saved;
        SaveVfxState(dev, saved);

        // World-space quads need world-space transforms. The engine draws with
        // vertex-shader constants and leaves the fixed-function matrices stale,
        // so without this every quad lands outside clip space (invisible).
        // WORLD and VIEW stay identity; the live per-eye view*proj (row-major,
        // captured from the eye block that called us) goes into PROJECTION.
        static const D3DMATRIX kIdentity = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
        dev->SetTransform(D3DTS_WORLD, &kIdentity);
        dev->SetTransform(D3DTS_VIEW, &kIdentity);
        dev->SetTransform(D3DTS_PROJECTION, reinterpret_cast<const D3DMATRIX*>(eyeProj));

        dev->SetVertexShader(nullptr);
        dev->SetPixelShader(nullptr);
        dev->SetVertexDeclaration(nullptr);
        dev->SetRenderState(D3DRS_LIGHTING, FALSE);
        dev->SetRenderState(D3DRS_FOGENABLE, FALSE);
        dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        dev->SetRenderState(D3DRS_ZENABLE, TRUE);
        dev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
        dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
        dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
        dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
        dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
        dev->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
        dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        dev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
        dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
        dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

        VfxVertex* verts = nullptr;
        if (FAILED(vb_->Lock(0, 0, reinterpret_cast<void**>(&verts), D3DLOCK_DISCARD)) || !verts) {
            RestoreVfxState(dev, saved);
            return;
        }
        int used = 0, quads = 0;
        IDirect3DTexture9* activeTex = nullptr;
        VfxBlend activeBlend = VfxBlend::Additive;
        bool haveActive = false;

        auto flush = [&]() {
            if (quads == 0 || !haveActive) return;
            vb_->Unlock();
            dev->SetStreamSource(0, vb_, 0, sizeof(VfxVertex));
            dev->SetFVF(kVfxFvf);
            dev->SetTexture(0, activeTex);
            dev->DrawPrimitive(D3DPT_TRIANGLELIST, 0, quads * 2);
            used = 0;
            quads = 0;
            if (FAILED(vb_->Lock(0, 0, reinterpret_cast<void**>(&verts), D3DLOCK_DISCARD)))
                verts = nullptr;
        };

        for (int i = 0; i < count && verts; ++i) {
            const VfxDrawItem& it = items[i];
            const VfxAtlas::ClipMap* clip = atlas.Lookup(it.clipId);
            if (!clip || clip->tex < 0 || clip->tex >= atlas.count) continue;
            IDirect3DTexture9* tex = atlas.texture[clip->tex];
            if (!tex) continue;
            const VfxBlend blend = static_cast<VfxBlend>(it.blend);

            if (!haveActive || activeTex != tex || activeBlend != blend || used + 6 > kMaxBatchVerts) {
                flush();
                if (!verts) break;
                if (!haveActive || activeTex != tex || activeBlend != blend) {
                    ApplyBlend(dev, blend);
                    activeTex = tex;
                    activeBlend = blend;
                    haveActive = true;
                }
            }

            const int cols = clip->cols > 0 ? clip->cols : 1;
            const int rows = clip->rows > 0 ? clip->rows : 1;
            int frame = it.frame;
            if (frame < 0) frame = 0;
            if (frame >= cols * rows) frame = cols * rows - 1;
            const int cx = frame % cols, cy = frame / cols;
            const float u0 = static_cast<float>(cx) / cols;
            const float v0 = static_cast<float>(cy) / rows;
            const float du = 1.0f / cols, dv = 1.0f / rows;

            const float s = it.size <= 0.0f ? 1.0f : it.size;
            const float rx = camRight[0] * s, ry = camRight[1] * s, rz = camRight[2] * s;
            const float ux = camUp[0] * s, uy = camUp[1] * s, uz = camUp[2] * s;
            const float px = it.position.x, py = it.position.y, pz = it.position.z;
            const D3DCOLOR c = D3DCOLOR_ARGB(255, 255, 255, 255);

            VfxVertex q[4];
            q[0] = { px - rx - ux, py - ry - uy, pz - rz - uz, c, u0,        v0 + dv };
            q[1] = { px + rx - ux, py + ry - uy, pz + rz - uz, c, u0 + du,   v0 + dv };
            q[2] = { px - rx + ux, py - ry + uy, pz - rz + uz, c, u0,        v0 };
            q[3] = { px + rx + ux, py + ry + uy, pz + rz + uz, c, u0 + du,   v0 };
            VfxVertex tri[6] = { q[0], q[1], q[2], q[2], q[1], q[3] };
            for (int k = 0; k < 6; ++k) verts[used + k] = tri[k];
            used += 6;
            ++quads;
        }

        if (verts) { flush(); }

        dev->SetTexture(0, nullptr);
        RestoreVfxState(dev, saved);
    }

private:
    static void ApplyBlend(IDirect3DDevice9* dev, VfxBlend blend) {
        dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        if (blend == VfxBlend::Additive) {
            dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
            dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
        } else {
            dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
            dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        }
    }

    IDirect3DVertexBuffer9* vb_ = nullptr;
};

} // namespace host
} // namespace singularity_vfx
