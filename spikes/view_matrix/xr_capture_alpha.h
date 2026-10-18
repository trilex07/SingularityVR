#pragma once
#include "glove_renderer.h"

namespace xrcapture {
// Final opaque XR image only. UE3's scene alpha is not display transparency:
// the measured source has A=0 across the world and nonzero A on HUD elements.
// Preserve RGB/depth and all engine bindings; never run inside a scene draw.
class OpaqueAlpha {
    IDirect3DStateBlock9* state=nullptr;
public:
    void Release(){gloves::Release(state);}
    bool Apply(IDirect3DDevice9* dev,IDirect3DSurface9* output,const gloves::Calls& calls){
        if(!dev||!output||!calls.Valid())return false;
        D3DSURFACE_DESC desc{};
        if(FAILED(output->GetDesc(&desc))||desc.Format!=D3DFMT_A8R8G8B8)return false;
        if(!state&&FAILED(dev->CreateStateBlock(D3DSBT_ALL,&state)))return false;
        gloves::StateGuard<> guard(dev,state,calls);if(!guard.valid)return false;
        // Begin before altering any state; failure means no pixel writes.
        if(FAILED(dev->BeginScene()))return false;
        bool ok=SUCCEEDED(dev->SetDepthStencilSurface(nullptr));
        for(unsigned i=1;i<guard.count;++i)ok=SUCCEEDED(calls.target(dev,i,nullptr))&&ok;
        const D3DVIEWPORT9 vp{0,0,desc.Width,desc.Height,0,1};
        ok=SUCCEEDED(calls.target(dev,0,output))&&ok;
        ok=SUCCEEDED(calls.viewport(dev,&vp))&&ok;
        ok=SUCCEEDED(calls.vs(dev,nullptr))&&ok;
        ok=SUCCEEDED(calls.ps(dev,nullptr))&&ok;
        ok=SUCCEEDED(calls.texture(dev,0,nullptr))&&ok;
        ok=SUCCEEDED(dev->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE))&&ok;
        const auto rs=[&](D3DRENDERSTATETYPE s,DWORD value){ok=SUCCEEDED(dev->SetRenderState(s,value))&&ok;};
        rs(D3DRS_ZENABLE,FALSE);rs(D3DRS_ZWRITEENABLE,FALSE);
        rs(D3DRS_STENCILENABLE,FALSE);rs(D3DRS_ALPHATESTENABLE,FALSE);
        rs(D3DRS_ALPHABLENDENABLE,FALSE);rs(D3DRS_SEPARATEALPHABLENDENABLE,FALSE);
        rs(D3DRS_SCISSORTESTENABLE,FALSE);rs(D3DRS_CULLMODE,D3DCULL_NONE);
        rs(D3DRS_FILLMODE,D3DFILL_SOLID);rs(D3DRS_FOGENABLE,FALSE);
        rs(D3DRS_LIGHTING,FALSE);rs(D3DRS_CLIPPLANEENABLE,0);
        rs(D3DRS_COLORWRITEENABLE,D3DCOLORWRITEENABLE_ALPHA);
        const auto ts=[&](DWORD stage,D3DTEXTURESTAGESTATETYPE s,DWORD value){ok=SUCCEEDED(dev->SetTextureStageState(stage,s,value))&&ok;};
        ts(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);ts(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);
        ts(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);ts(0,D3DTSS_ALPHAARG1,D3DTA_DIFFUSE);
        ts(1,D3DTSS_COLOROP,D3DTOP_DISABLE);ts(1,D3DTSS_ALPHAOP,D3DTOP_DISABLE);
        struct Vertex{float x,y,z,rhw;DWORD color;};
        const float w=(float)desc.Width-.5f,h=(float)desc.Height-.5f;
        const Vertex v[4]={{-.5f,-.5f,0,1,0xff000000},{w,-.5f,0,1,0xff000000},
            {w,h,0,1,0xff000000},{-.5f,h,0,1,0xff000000}};
        const unsigned short indices[6]={0,1,2,0,2,3};
        if(ok)ok=SUCCEEDED(calls.draw(dev,D3DPT_TRIANGLELIST,0,4,2,indices,D3DFMT_INDEX16,v,sizeof(Vertex)));
        ok=SUCCEEDED(dev->EndScene())&&ok;
        return guard.Restore()&&ok;
    }
};
}
