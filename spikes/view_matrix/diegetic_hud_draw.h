#pragma once
#include "diegetic_weapon_hud.h"
#include <d3d9.h>

namespace diegetichud {
struct ScreenVertex { float x,y,z,rhw; DWORD color; float u,v; };
struct EyePlan {
    ScreenVertex quad[16][4]{}; // triangle strip order TL,TR,BL,BR
    bool front[16]{};
    float minX=1e9f,minY=1e9f,maxX=-1e9f,maxY=-1e9f;
};
struct DrawPlan {
    EyePlan eye[2];
    unsigned width=0,height=0;
    int count=0;
    bool ready=false;
};

// Preflight BOTH eyes before touching the backbuffer. A projection rejection
// must not leave half a 3D HUD underneath the legacy fallback.
inline DrawPlan Project(const Quad3D* quads,int count,const Pose& pose,
    unsigned width,unsigned height,unsigned alphaPct) {
    DrawPlan plan;plan.width=width;plan.height=height;plan.count=count;
    if(!quads || count<6 || count>16 || width<8 || height<8 ||
       !Finite(pose.chain) || !Finite(pose.eyeClip[0]) || !Finite(pose.eyeClip[1])) return plan;
    for(int eye=0;eye<2;++eye) {
        auto& ep=plan.eye[eye];
        const auto& m=pose.eyeClip[eye].m;
        for(int q=0;q<count;++q) {
            ScreenVertex corners[4];
            for(int k=0;k<4;++k) {
                const auto p=Point(quads[q].p[k],pose.chain);
                float cl[4];
                for(int c=0;c<4;++c) cl[c]=p.x*m[0][c]+p.y*m[1][c]+p.z*m[2][c]+m[3][c];
                if(!std::isfinite(cl[0]) || !std::isfinite(cl[1]) ||
                   !std::isfinite(cl[2]) || !std::isfinite(cl[3]) || cl[3]<=.001f) return plan;
                auto& v=corners[k];v.rhw=1.f/cl[3];
                v.x=(cl[0]*v.rhw*.5f+.5f)*width;
                v.y=(.5f-cl[1]*v.rhw*.5f)*height;
                v.z=(std::max)(0.f,(std::min)(1.f,cl[2]*v.rhw));
                if(!std::isfinite(v.x) || !std::isfinite(v.y)) return plan;
                v.color=(quads[q].color&0x00ffffffu)|
                    (((quads[q].color>>24)*(std::min)(alphaPct,100u)/100u)<<24);
                v.u=(k==0||k==3)?quads[q].u0:quads[q].u1;
                v.v=k<2?quads[q].v0:quads[q].v1;
                ep.minX=(std::min)(ep.minX,v.x);ep.maxX=(std::max)(ep.maxX,v.x);
                ep.minY=(std::min)(ep.minY,v.y);ep.maxY=(std::max)(ep.maxY,v.y);
            }
            // Diagnostic signed winding, not pixel visibility. Preserve the
            // baseline double-sided draw until runtime occlusion is measured.
            const float area=(corners[1].x-corners[0].x)*(corners[3].y-corners[0].y)-
                             (corners[1].y-corners[0].y)*(corners[3].x-corners[0].x);
            ep.front[q]=area>0.f;
            ep.quad[q][0]=corners[0];ep.quad[q][1]=corners[1];
            ep.quad[q][2]=corners[3];ep.quad[q][3]=corners[2];
        }
        if(!EyeBoundsVisible(ep.minX,ep.minY,ep.maxX,ep.maxY,eye,width,height)) return plan;
    }
    plan.ready=true;return plan;
}

struct DrawResult {
    HRESULT hr=E_FAIL;
    unsigned handledEyes=0;
    unsigned submitted[2]{};
};

// Called with original (unhooked) SetRenderTarget/DrawPrimitiveUP entrypoints.
// The offline D3D9/Ex pixel test executes this exact state and submission path.
template<class SetTarget,class Draw>
DrawResult Render(IDirect3DDevice9* dev,IDirect3DSurface9* back,IDirect3DTexture9* atlas,
    IDirect3DStateBlock9*& state,const DrawPlan& plan,SetTarget setTarget,Draw draw) {
    DrawResult result;
    if(!dev || !back || !atlas || !plan.ready) return result;
    IDirect3DSurface9 *old=nullptr,*depth=nullptr;
    if(FAILED(dev->GetRenderTarget(0,&old))) return result;
    const HRESULT dh=dev->GetDepthStencilSurface(&depth);
    if(FAILED(dh) && dh!=D3DERR_NOTFOUND){old->Release();return result;}
    if(!state && FAILED(dev->CreateStateBlock(D3DSBT_ALL,&state))){
        old->Release();if(depth)depth->Release();return result;
    }
    if(FAILED(state->Capture())){old->Release();if(depth)depth->Release();return result;}
    HRESULT hr=S_OK;
    auto check=[&](HRESULT h){if(FAILED(h)&&SUCCEEDED(hr))hr=h;};
    check(setTarget(back));check(dev->SetDepthStencilSurface(nullptr));
    D3DVIEWPORT9 vp{0,0,plan.width,plan.height,0.f,1.f};
    check(dev->SetViewport(&vp));
    check(dev->SetVertexShader(nullptr));check(dev->SetPixelShader(nullptr));
    const struct { D3DRENDERSTATETYPE key;DWORD value; } states[]={
        {D3DRS_ZENABLE,D3DZB_FALSE},{D3DRS_ZWRITEENABLE,FALSE},{D3DRS_STENCILENABLE,FALSE},
        {D3DRS_ALPHATESTENABLE,FALSE},{D3DRS_FOGENABLE,FALSE},{D3DRS_LIGHTING,FALSE},
        {D3DRS_ALPHABLENDENABLE,TRUE},{D3DRS_SRCBLEND,D3DBLEND_SRCALPHA},
        {D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA},{D3DRS_BLENDOP,D3DBLENDOP_ADD},
        {D3DRS_SEPARATEALPHABLENDENABLE,FALSE},{D3DRS_CULLMODE,D3DCULL_NONE},
        {D3DRS_FILLMODE,D3DFILL_SOLID},{D3DRS_COLORWRITEENABLE,15},
        {D3DRS_SCISSORTESTENABLE,TRUE},{D3DRS_CLIPPLANEENABLE,0}};
    for(const auto& s:states)check(dev->SetRenderState(s.key,s.value));
    check(dev->SetTexture(0,atlas));
    check(dev->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE));
    check(dev->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE));
    check(dev->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_DIFFUSE));
    check(dev->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_MODULATE));
    check(dev->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE));
    check(dev->SetTextureStageState(0,D3DTSS_ALPHAARG2,D3DTA_DIFFUSE));
    check(dev->SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,0));
    check(dev->SetTextureStageState(0,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_DISABLE));
    check(dev->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE));
    check(dev->SetTextureStageState(1,D3DTSS_ALPHAOP,D3DTOP_DISABLE));
    check(dev->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR));
    check(dev->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR));
    check(dev->SetSamplerState(0,D3DSAMP_MIPFILTER,D3DTEXF_NONE));
    check(dev->SetSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP));
    check(dev->SetSamplerState(0,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP));
    check(dev->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1));
    for(int eye=0;eye<2 && SUCCEEDED(hr);++eye) {
        RECT rect{eye?LONG(plan.width/2):0,0,eye?LONG(plan.width):LONG(plan.width/2),LONG(plan.height)};
        check(dev->SetScissorRect(&rect));
        for(int q=0;q<plan.count && SUCCEEDED(hr);++q) {
            check(draw(plan.eye[eye].quad[q]));
            if(SUCCEEDED(hr))++result.submitted[eye];
        }
    }
    // SetRenderTarget resets the viewport; restore targets BEFORE applying the
    // saved state (including viewport, scissor, texture stages and stream 0).
    check(setTarget(old));check(dev->SetDepthStencilSurface(depth));check(state->Apply());
    old->Release();if(depth)depth->Release();
    result.hr=hr;
    // Successful API submission is not physical/pixel visibility evidence.
    if(SUCCEEDED(hr))result.handledEyes=3;
    return result;
}
} // namespace diegetichud
