#pragma once
#include <d3d9.h>
#include <algorithm>
namespace startup {
// Present-only TARGET clears. No shader/material/depth-content/input writes.
// Caller supplies original setters to avoid render-hook classification changes.
template<class SetTarget,class SetViewport,class Clear,class Text>
bool DrawLoading(IDirect3DDevice9* dev,SetTarget target,SetViewport viewport,Clear clear,Text text,bool& restored,bool preserveScene=false){
    restored=true;if(!dev)return false;
    IDirect3DSurface9 *back=nullptr,*old=nullptr,*depth=nullptr;
    D3DSURFACE_DESC desc{};D3DVIEWPORT9 saved{};DWORD scissor=0;RECT savedScissor{};
    if(FAILED(dev->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back))||!back)return false;
    const HRESULT depthResult=dev->GetDepthStencilSurface(&depth);
    const bool valid=SUCCEEDED(back->GetDesc(&desc))&&desc.Width>=64&&desc.Height>=32&&
        SUCCEEDED(dev->GetRenderTarget(0,&old))&&old&&SUCCEEDED(dev->GetViewport(&saved))&&
        SUCCEEDED(dev->GetScissorRect(&savedScissor))&&
        SUCCEEDED(dev->GetRenderState(D3DRS_SCISSORTESTENABLE,&scissor))&&
        (SUCCEEDED(depthResult)||depthResult==D3DERR_NOTFOUND);
    bool drawn=false;
    if(valid){
        const D3DVIEWPORT9 full{0,0,desc.Width,desc.Height,0.f,1.f};
        if(SUCCEEDED(dev->SetDepthStencilSurface(nullptr))&&SUCCEEDED(target(dev,0,back))&&
           SUCCEEDED(viewport(dev,&full))&&SUCCEEDED(dev->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE))&&
           (preserveScene || SUCCEEDED(clear(dev,0,nullptr,D3DCLEAR_TARGET,0xff080c14,1.f,0)))){
            D3DRECT rects[1536];int n=0;
            const int half=(int)desc.Width/2;
            const int px=(std::max)(1,(std::min)(half/(preserveScene?240:160),(int)desc.Height/100));
            for(int eye=0;eye<2;++eye)n=text(rects,n,1536,eye*half+(half-13*6*px)/2,
                (preserveScene?(int)desc.Height*3/4:(int)desc.Height/2)-4*px,px,
                preserveScene?"VR INIT...":"WCZYTYWANIE VR");
            drawn=n>0;
            for(int at=0;at<n;at+=512)if(FAILED(clear(dev,(DWORD)(std::min)(512,n-at),rects+at,
                D3DCLEAR_TARGET,0xffc8d9e8,1.f,0)))drawn=false;
        }
        // SetTarget resets viewport. Restore RT/depth first, viewport LAST.
        const HRESULT rt=target(dev,0,old),ds=dev->SetDepthStencilSurface(depth);
        const HRESULT vp=viewport(dev,&saved),rect=dev->SetScissorRect(&savedScissor);
        const HRESULT sc=dev->SetRenderState(D3DRS_SCISSORTESTENABLE,scissor);
        restored=SUCCEEDED(rt)&&SUCCEEDED(ds)&&SUCCEEDED(vp)&&SUCCEEDED(rect)&&SUCCEEDED(sc);
    }
    if(depth)depth->Release();if(old)old->Release();back->Release();
    return drawn&&restored;
}
} // namespace startup
