#pragma once
#include <d3d9.h>
#include "frontend_menu_comfort.h"
#include "cinema_surface_copy.h"

namespace frontmenu {
struct Vertex {float x,y,z,rhw;DWORD colour;float u,v;};
using DrawUP=HRESULT (STDMETHODCALLTYPE*)(IDirect3DDevice9*,D3DPRIMITIVETYPE,UINT,const void*,UINT);
using SetVS=HRESULT (STDMETHODCALLTYPE*)(IDirect3DDevice9*,IDirect3DVertexShader9*);
using SetTexture=HRESULT (STDMETHODCALLTYPE*)(IDirect3DDevice9*,DWORD,IDirect3DBaseTexture9*);
struct Resources {
    IDirect3DTexture9* texture=nullptr;
    IDirect3DSurface9* saved=nullptr;
    IDirect3DStateBlock9* state=nullptr;
    UINT width=0,height=0;D3DFORMAT format=D3DFMT_UNKNOWN;
    bool fault=false,allocationFailed=false;
    void Release(){
        if(saved)saved->Release();if(texture)texture->Release();if(state)state->Release();
        saved=nullptr;texture=nullptr;state=nullptr;width=height=0;format=D3DFMT_UNKNOWN;
        fault=allocationFailed=false;
    }
    bool Ensure(IDirect3DDevice9* dev,const D3DSURFACE_DESC& d){
        if(width!=d.Width||height!=d.Height||format!=d.Format){Release();width=d.Width;height=d.Height;format=d.Format;}
        if(fault||allocationFailed)return false;
        if(!saved && FAILED(CreateCinemaCopyTarget(dev,d,&texture,&saved))){allocationFailed=true;return false;}
        if(!state && FAILED(dev->CreateStateBlock(D3DSBT_ALL,&state))){allocationFailed=true;return false;}
        return state&&saved;
    }
};
// Present only, OUTSIDE BeginScene. The saved full SBS buffer is reused until
// Reset/size change. ColorFill targets only back, never a bound auxiliary MRT.
struct SurfacePass {
    IDirect3DDevice9* dev;IDirect3DSurface9* back;IDirect3DTexture9* menu;
    Resources& resources;const D3DSURFACE_DESC& desc;const Vertex (&vertices)[2][4];
    CinemaSetTarget setTarget;SetVS setVS;SetTexture setTexture;DrawUP draw;
    IDirect3DSurface9 *old=nullptr,*depth=nullptr,*extra[3]{};
    D3DVIEWPORT9 viewport{};DWORD targets=1;
    SurfacePass(IDirect3DDevice9* d,IDirect3DSurface9* b,IDirect3DTexture9* m,Resources& r,
        const D3DSURFACE_DESC& dc,const Vertex (&v)[2][4],CinemaSetTarget rt,SetVS vs,SetTexture tex,DrawUP up)
        :dev(d),back(b),menu(m),resources(r),desc(dc),vertices(v),setTarget(rt),setVS(vs),setTexture(tex),draw(up){}
    ~SurfacePass(){if(old)old->Release();if(depth)depth->Release();for(auto* e:extra)if(e)e->Release();}
    bool Capture(){
        if(!dev||!back||!menu||!setTarget||!setVS||!draw||!resources.Ensure(dev,desc))return false;
        D3DCAPS9 caps{};
        if(FAILED(dev->GetDeviceCaps(&caps))||FAILED(dev->GetRenderTarget(0,&old))||!old)return false;
        targets=caps.NumSimultaneousRTs>4?4:caps.NumSimultaneousRTs;
        HRESULT hr=dev->GetDepthStencilSurface(&depth);
        if(FAILED(hr)&&hr!=D3DERR_NOTFOUND)return false;
        for(DWORD i=1;i<targets;++i){hr=dev->GetRenderTarget(i,&extra[i-1]);if(FAILED(hr)&&hr!=D3DERR_NOTFOUND)return false;}
        return SUCCEEDED(dev->GetViewport(&viewport))&&SUCCEEDED(resources.state->Capture());
    }
    bool SavePixels(){return SUCCEEDED(dev->StretchRect(back,nullptr,resources.saved,nullptr,D3DTEXF_NONE));}
    bool Setup(){
        bool ok=true;auto check=[&](HRESULT hr){if(FAILED(hr))ok=false;};
        check(dev->SetDepthStencilSurface(nullptr));
        for(DWORD i=1;i<targets;++i)check(setTarget(dev,i,nullptr));
        check(setTarget(dev,0,back));D3DVIEWPORT9 vp{0,0,desc.Width,desc.Height,0,1};check(dev->SetViewport(&vp));
        check(setVS(dev,nullptr));check(dev->SetPixelShader(nullptr));
        check(dev->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1));
        check(dev->SetRenderState(D3DRS_ZENABLE,FALSE));check(dev->SetRenderState(D3DRS_ZWRITEENABLE,FALSE));
        check(dev->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE));
        check(dev->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA));
        check(dev->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA));
        check(dev->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD));
        check(dev->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE,FALSE));
        check(dev->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE));check(dev->SetRenderState(D3DRS_STENCILENABLE,FALSE));
        check(dev->SetRenderState(D3DRS_FOGENABLE,FALSE));check(dev->SetRenderState(D3DRS_LIGHTING,FALSE));
        check(dev->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE));
        check(dev->SetRenderState(D3DRS_SCISSORTESTENABLE,TRUE));check(dev->SetRenderState(D3DRS_COLORWRITEENABLE,15));
        check(dev->SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,0));
        check(dev->SetTextureStageState(0,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_DISABLE));
        check(dev->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE));
        check(dev->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE));check(dev->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_DIFFUSE));
        check(dev->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_MODULATE));
        check(dev->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE));check(dev->SetTextureStageState(0,D3DTSS_ALPHAARG2,D3DTA_DIFFUSE));
        check(dev->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE));check(dev->SetTextureStageState(1,D3DTSS_ALPHAOP,D3DTOP_DISABLE));
        check(dev->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR));check(dev->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR));
        check(dev->SetSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP));check(dev->SetSamplerState(0,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP));
        check(setTexture?setTexture(dev,0,menu):dev->SetTexture(0,menu));
        return ok;
    }
    bool Clear(){return SUCCEEDED(dev->ColorFill(back,nullptr,0xFF080C10));}
    bool BeginScene(){return SUCCEEDED(dev->BeginScene());}
    bool DrawEye(int eye){
        const RECT rect{(LONG)(eye*desc.Width/2),0,(LONG)((eye+1)*desc.Width/2),(LONG)desc.Height};
        return SUCCEEDED(dev->SetScissorRect(&rect))&&SUCCEEDED(draw(dev,D3DPT_TRIANGLESTRIP,2,vertices[eye],sizeof(Vertex)));
    }
    bool EndScene(){return SUCCEEDED(dev->EndScene());}
    bool Restore(){
        bool ok=true;auto check=[&](HRESULT hr){if(FAILED(hr))ok=false;};
        check(setTarget(dev,0,old));for(DWORD i=1;i<targets;++i)check(setTarget(dev,i,extra[i-1]));
        check(dev->SetDepthStencilSurface(depth));check(resources.state->Apply());check(dev->SetViewport(&viewport));
        return ok;
    }
    bool RestorePixels(){return SUCCEEDED(dev->StretchRect(resources.saved,nullptr,back,nullptr,D3DTEXF_NONE));}
};
} // namespace frontmenu
