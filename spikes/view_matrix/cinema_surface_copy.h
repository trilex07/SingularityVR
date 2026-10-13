#pragma once
#include <d3d9.h>
#include "cutscene_mono.h"

using CinemaSetTarget = HRESULT (STDMETHODCALLTYPE*)(IDirect3DDevice9*,DWORD,IDirect3DSurface9*);
using CinemaClear = HRESULT (STDMETHODCALLTYPE*)(IDirect3DDevice9*,DWORD,const D3DRECT*,DWORD,D3DCOLOR,float,DWORD);

// Mirror the already working zero-copy capture destination: DEFAULT-pool
// render-target texture, same format, one mip, no shared handle required here.
inline HRESULT CreateCinemaCopyTarget(IDirect3DDevice9* dev, const D3DSURFACE_DESC& d,
    IDirect3DTexture9** texture, IDirect3DSurface9** surface) {
    if (!dev || !texture || !surface || !d.Width || !d.Height) return E_INVALIDARG;
    *texture=nullptr; *surface=nullptr;
    HRESULT hr=dev->CreateTexture(d.Width,d.Height,1,D3DUSAGE_RENDERTARGET,d.Format,
        D3DPOOL_DEFAULT,texture,nullptr);
    if (SUCCEEDED(hr) && *texture) hr=(*texture)->GetSurfaceLevel(0,surface);
    if (FAILED(hr) || !*surface) {
        if (*surface) { (*surface)->Release(); *surface=nullptr; }
        if (*texture) { (*texture)->Release(); *texture=nullptr; }
        return FAILED(hr)?hr:E_POINTER;
    }
    return S_OK;
}

struct CinemaCopyResult {
    HRESULT hr=S_OK;
    const char* stage="complete";
    bool stateRestored=true, pixelsRestored=true;
};

// Shared by production and the synthetic GPU pixel/state test. Present-side
// only, outside BeginScene; no hooks, allocations, logging or readback here.
// Keep original pixels in saved before any clears, and restore them on failure.
inline CinemaCopyResult ComposeCinemaSurface(IDirect3DDevice9* dev, IDirect3DSurface9* back,
    IDirect3DSurface9* saved, const D3DSURFACE_DESC& d, const CutsceneMonoFit& fit,
    CinemaSetTarget setTarget, CinemaClear clear) {
    CinemaCopyResult r;
    if (!dev || !back || !saved || back==saved || !fit.valid || fit.barCount<0 || fit.barCount>4 || !setTarget || !clear)
        return {E_INVALIDARG,"arguments"};
    const RECT full{0,0,(LONG)d.Width,(LONG)d.Height};
    HRESULT hr=dev->StretchRect(back,nullptr,saved,nullptr,D3DTEXF_NONE);
    if (FAILED(hr)) return {hr,"copy-source-to-temp"};
    IDirect3DSurface9 *old=nullptr,*depth=nullptr;
    IDirect3DSurface9* extra[3]{};
    D3DCAPS9 caps{};
    D3DVIEWPORT9 viewport{}; DWORD scissor=FALSE;
    hr=dev->GetRenderTarget(0,&old);
    if (FAILED(hr) || !old) {if(old)old->Release();return {FAILED(hr)?hr:E_POINTER,"save-target"};}
    const HRESULT depthHr=dev->GetDepthStencilSurface(&depth);
    if (FAILED(depthHr) && depthHr!=D3DERR_NOTFOUND) r={depthHr,"save-depth"};
    if (SUCCEEDED(r.hr)) { hr=dev->GetViewport(&viewport); if(FAILED(hr)) r={hr,"save-viewport"}; }
    if (SUCCEEDED(r.hr)) { hr=dev->GetRenderState(D3DRS_SCISSORTESTENABLE,&scissor); if(FAILED(hr)) r={hr,"save-scissor"}; }
    if (SUCCEEDED(r.hr)) { hr=dev->GetDeviceCaps(&caps); if(FAILED(hr)) r={hr,"save-caps"}; }
    const DWORD targets=caps.NumSimultaneousRTs>4?4:caps.NumSimultaneousRTs;
    for(DWORD i=1;SUCCEEDED(r.hr)&&i<targets;++i) {
        hr=dev->GetRenderTarget(i,&extra[i-1]);
        if(FAILED(hr)&&hr!=D3DERR_NOTFOUND)r={hr,"save-extra-target"};
    }
    if (FAILED(r.hr)) {
        old->Release(); if(depth)depth->Release();
        for(auto* surface:extra)if(surface)surface->Release();
        return r;
    }
    bool pixelsTouched=false;
    hr=dev->SetDepthStencilSurface(nullptr); if(FAILED(hr))r={hr,"unbind-depth"};
    // Clear affects every bound render target: isolate the backbuffer, not the
    // engine's auxiliary MRT surfaces. Restore their identities below.
    for(DWORD i=1;SUCCEEDED(r.hr)&&i<targets;++i) {
        hr=setTarget(dev,i,nullptr); if(FAILED(hr))r={hr,"unbind-extra-target"};
    }
    if (SUCCEEDED(r.hr)) { hr=setTarget(dev,0,back); if(FAILED(hr))r={hr,"bind-back"}; }
    D3DVIEWPORT9 fullViewport{0,0,d.Width,d.Height,0.f,1.f};
    if (SUCCEEDED(r.hr)) { hr=dev->SetViewport(&fullViewport); if(FAILED(hr))r={hr,"full-viewport"}; }
    if (SUCCEEDED(r.hr)) { hr=dev->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE); if(FAILED(hr))r={hr,"disable-scissor"}; }
    if (SUCCEEDED(r.hr) && fit.barCount) {
        D3DRECT bars[4];
        for(int i=0;i<fit.barCount;++i)bars[i]={fit.bars[i].left,fit.bars[i].top,fit.bars[i].right,fit.bars[i].bottom};
        pixelsTouched=true;
        hr=clear(dev,(DWORD)fit.barCount,bars,D3DCLEAR_TARGET,D3DCOLOR_XRGB(0,0,0),1.f,0);
        if(FAILED(hr))r={hr,"clear-bars"};
    }
    if (SUCCEEDED(r.hr)) {
        const RECT image{fit.image.left,fit.image.top,fit.image.right,fit.image.bottom};
        pixelsTouched=true;
        hr=dev->StretchRect(saved,&full,back,&image,D3DTEXF_LINEAR);
        if(FAILED(hr))r={hr,"fit-temp-to-back"};
    }
    HRESULT stateError=S_OK;
    auto restore=[&](HRESULT value){if(FAILED(value)&&SUCCEEDED(stateError))stateError=value;};
    restore(setTarget(dev,0,old));
    for(DWORD i=1;i<targets;++i)restore(setTarget(dev,i,extra[i-1]));
    restore(dev->SetDepthStencilSurface(depth));
    restore(dev->SetViewport(&viewport));
    restore(dev->SetRenderState(D3DRS_SCISSORTESTENABLE,scissor));
    if (FAILED(stateError)) { r.hr=stateError; r.stage="restore-state"; r.stateRestored=false; }
    if (FAILED(r.hr) && pixelsTouched) {
        hr=dev->StretchRect(saved,&full,back,&full,D3DTEXF_NONE);
        if (FAILED(hr)) { r.hr=hr; r.stage="restore-pixels"; r.pixelsRestored=false; }
    }
    old->Release(); if(depth)depth->Release();
    for(auto* surface:extra)if(surface)surface->Release();
    return r;
}
