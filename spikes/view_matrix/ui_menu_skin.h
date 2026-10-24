#pragma once
#include <d3d9.h>
#include <cmath>
namespace menuskin {
inline bool Eligible(bool enabled,int menuClass,bool modal,bool settings){
    return enabled&&!modal&&!settings&&(menuClass==2||menuClass==3);
}
// Use the native projected quad, not a second screen-space menu or hit map.
template<class V> bool Geometry(const V (&native)[4],V (&out)[4]){
    float cx=0,cy=0;for(const auto& v:native){if(!std::isfinite(v.x)||!std::isfinite(v.y))return false;cx+=v.x*.25f;cy+=v.y*.25f;}
    for(unsigned i=0;i<4;++i){out[i]=native[i];
        out[i].x=cx+(native[i].x-cx)*1.035f;out[i].y=cy+(native[i].y-cy)*1.035f;
        out[i].u=((i&1)?1892.f:1212.f)/4096.f;
        out[i].v=((i&2)?844.f:4.f)/2048.f;
    }
    return true;
}
// Called inside existing captured state, before native pixels. Always restore
// blend/texture locally, including failed draws. Outer transaction owns RT/IB.
template<class V,class Draw> bool DrawBackdrop(IDirect3DDevice9* dev,IDirect3DTexture9* atlas,
                                              const V (&native)[4],Draw draw){
    if(!dev||!atlas||!draw)return false;V v[4]{};if(!Geometry(native,v))return false;
    DWORD src=0,dst=0;IDirect3DBaseTexture9* old=nullptr;
    if(FAILED(dev->GetRenderState(D3DRS_SRCBLEND,&src))||FAILED(dev->GetRenderState(D3DRS_DESTBLEND,&dst))||FAILED(dev->GetTexture(0,&old)))return false;
    bool ok=SUCCEEDED(dev->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA));
    ok=SUCCEEDED(dev->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA))&&ok;
    ok=SUCCEEDED(dev->SetTexture(0,atlas))&&ok;
    if(ok)ok=SUCCEEDED(draw(dev,D3DPT_TRIANGLESTRIP,2,v,sizeof(V)));
    bool restored=SUCCEEDED(dev->SetTexture(0,old));if(old)old->Release();
    restored=SUCCEEDED(dev->SetRenderState(D3DRS_SRCBLEND,src))&&restored;
    restored=SUCCEEDED(dev->SetRenderState(D3DRS_DESTBLEND,dst))&&restored;
    return ok&&restored;
}
}
