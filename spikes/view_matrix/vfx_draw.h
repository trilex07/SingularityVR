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
                   const VfxAtlas& atlas, const float camRight[3], const float camUp[3]) {
        if (!dev || !items || count <= 0 || !vb_) return;

        VfxSavedState saved;
        SaveVfxState(dev, saved);

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
