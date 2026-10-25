#pragma once
#include <d3d9.h>

// The wheel is a fixed-function overlay, not a continuation of the weapon's
// shader/material. Caller owns D3DSBT_ALL + RT/DS/viewport restoration. No asset,
// input, projection or native GFx changes. Shared by production and pixel tests.
namespace wheelstate {
enum class Pass { Legacy, Atlas, Composite };
inline bool Prepare(IDirect3DDevice9* d,Pass pass) {
    if(!d)return false;
    bool ok=true;
    auto rs=[&](D3DRENDERSTATETYPE s,DWORD v){ok=SUCCEEDED(d->SetRenderState(s,v))&&ok;};
    auto ts=[&](DWORD stage,D3DTEXTURESTAGESTATETYPE s,DWORD v){ok=SUCCEEDED(d->SetTextureStageState(stage,s,v))&&ok;};
    auto ss=[&](D3DSAMPLERSTATETYPE s,DWORD v){ok=SUCCEEDED(d->SetSamplerState(0,s,v))&&ok;};
    ok=SUCCEEDED(d->SetVertexShader(nullptr))&&ok;
    ok=SUCCEEDED(d->SetPixelShader(nullptr))&&ok;
    rs(D3DRS_ZENABLE,FALSE);rs(D3DRS_ZWRITEENABLE,FALSE);
    rs(D3DRS_STENCILENABLE,FALSE);rs(D3DRS_ALPHATESTENABLE,FALSE);
    rs(D3DRS_FOGENABLE,FALSE);rs(D3DRS_LIGHTING,FALSE);rs(D3DRS_SPECULARENABLE,FALSE);
    rs(D3DRS_SCISSORTESTENABLE,FALSE);rs(D3DRS_CULLMODE,D3DCULL_NONE);
    rs(D3DRS_FILLMODE,D3DFILL_SOLID);rs(D3DRS_CLIPPLANEENABLE,0);
    rs(D3DRS_COLORWRITEENABLE,15);rs(D3DRS_SRGBWRITEENABLE,FALSE);rs(D3DRS_WRAP0,0);
    rs(D3DRS_ALPHABLENDENABLE,pass!=Pass::Legacy);
    rs(D3DRS_BLENDOP,D3DBLENDOP_ADD);rs(D3DRS_BLENDOPALPHA,D3DBLENDOP_ADD);
    rs(D3DRS_SRCBLEND,pass==Pass::Composite?D3DBLEND_ONE:D3DBLEND_SRCALPHA);
    rs(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
    // Atlas -> transparent scratch: RGB becomes premultiplied, A stays straight.
    rs(D3DRS_SEPARATEALPHABLENDENABLE,pass==Pass::Atlas);
    rs(D3DRS_SRCBLENDALPHA,D3DBLEND_ONE);rs(D3DRS_DESTBLENDALPHA,D3DBLEND_INVSRCALPHA);
    ts(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);ts(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);
    ts(0,D3DTSS_COLORARG1,pass==Pass::Legacy?D3DTA_DIFFUSE:D3DTA_TEXTURE);
    ts(0,D3DTSS_ALPHAARG1,pass==Pass::Legacy?D3DTA_DIFFUSE:D3DTA_TEXTURE);
    ts(0,D3DTSS_TEXCOORDINDEX,0);ts(0,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_DISABLE);
    // Terminate the inherited multi-texture material after the UI atlas.
    ts(1,D3DTSS_COLOROP,D3DTOP_DISABLE);ts(1,D3DTSS_ALPHAOP,D3DTOP_DISABLE);
    ss(D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP);ss(D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP);
    ss(D3DSAMP_MINFILTER,D3DTEXF_LINEAR);ss(D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);
    ss(D3DSAMP_MIPFILTER,D3DTEXF_NONE);ss(D3DSAMP_SRGBTEXTURE,FALSE);
    return ok;
}
}
