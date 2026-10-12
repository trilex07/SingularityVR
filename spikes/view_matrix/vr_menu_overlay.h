#pragma once
// Included after the D3D state/overlay helpers. No per-frame allocation or scan.
#include "menu_draw_trace.h"
#include "pause_menu_head_overlay.h"
#include "prompt_roles.h"
#include "prompt_atlas_generated.h"
extern volatile LONG g_pauseHeadMenu;
extern volatile LONG g_pauseMenuMode;
extern volatile LONG g_pauseCaptureOk;
extern volatile LONG g_pauseModeRedirects;
extern volatile LONG g_pauseModeStretches;
extern volatile LONG g_pauseModeBakes;
extern volatile LONG g_gamePaused;
extern volatile LONG g_vfxFrame;
#include "pause_menu_mode.h"
#include "pause_canvas_capture.h"
static uintptr_t g_vrMenuSurface=0;
static unsigned g_vrMenuFrame=0,g_vrMenuTextureFrame=0;
static IDirect3DTexture9* g_pauseHeadPoolTex=nullptr;
static IDirect3DSurface9* g_pauseHeadPoolSurf=nullptr;
static unsigned g_pauseHeadPoolClearFrame=~0u;
inline bool PauseHeadGfxBatchEligible(bool menuLayout) {
    return pausehead::GfxPauseMenuMonoBatch((int)Rd(g_vrMenuClass), menuLayout,
                                            Rd(g_vrUiNativeReady) != 0, Rd(g_gamePaused) != 0,
                                            Rd(g_rtIsScene) != 0, Rd(g_rtIsBackbuffer) != 0,
                                            pausehead::IniEnabled((int)Rd(g_pauseHeadMenu)), VrModeOn(),
                                            Rd(g_vrMenuOverlay) != 0, Rd(g_panelOpen) != 0);
}
static bool EnsurePauseHeadPool(IDirect3DDevice9* dev) {
    if (g_pauseHeadPoolTex && g_pauseHeadPoolSurf) return true;
    IDirect3DTexture9* tex=nullptr;
    const HRESULT hr=g_origCreateTexture
        ? g_origCreateTexture(dev,pausehead::kPoolWidth,pausehead::kPoolHeight,1,D3DUSAGE_RENDERTARGET,
                              D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&tex,nullptr)
        : dev->CreateTexture(pausehead::kPoolWidth,pausehead::kPoolHeight,1,D3DUSAGE_RENDERTARGET,
                             D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&tex,nullptr);
    if (FAILED(hr) || !tex) return false;
    IDirect3DSurface9* surf=nullptr;
    if (FAILED(tex->GetSurfaceLevel(0,&surf))) { tex->Release(); return false; }
    g_pauseHeadPoolTex=tex; g_pauseHeadPoolSurf=surf;
    Log("PauseHeadMenu: pooled GFx target %ux%u ready",pausehead::kPoolWidth,pausehead::kPoolHeight);
    return true;
}
template<typename DrawFn>
static HRESULT PauseHeadOverlayHudDraw(IDirect3DDevice9* dev,DrawFn draw,unsigned frameClearKey) {
    const pausehead::RedirectPlan plan=pausehead::PlanGfxRedirect(
        g_pauseHeadPoolClearFrame, frameClearKey, true, EnsurePauseHeadPool(dev));
    if (!plan.attemptRedirect) return draw();
    IDirect3DSurface9* oldRt=nullptr;
    if (FAILED(dev->GetRenderTarget(0,&oldRt)) || !oldRt) return draw();
    D3DVIEWPORT9 oldVp{};
    if (FAILED(dev->GetViewport(&oldVp))) {
        oldRt->Release();
        return draw();
    }
    pausehead::BindStep bindStep=pausehead::BindStep::Ok;
    if (FAILED(g_origSetRenderTarget(dev,0,g_pauseHeadPoolSurf)))
        bindStep=pausehead::BindStep::SetRenderTargetFailed;
    D3DVIEWPORT9 vp{0,0,pausehead::kPoolWidth,pausehead::kPoolHeight,0,1};
    if (bindStep==pausehead::BindStep::Ok && FAILED(dev->SetViewport(&vp)))
        bindStep=pausehead::BindStep::SetViewportFailed;
    if (bindStep==pausehead::BindStep::Ok && plan.clearPool &&
        FAILED(dev->Clear(0,nullptr,D3DCLEAR_TARGET,D3DCOLOR_ARGB(0,0,0,0),1.f,0)))
        bindStep=pausehead::BindStep::ClearFailed;
    if (bindStep!=pausehead::BindStep::Ok) {
        g_origSetRenderTarget(dev,0,oldRt);
        dev->SetViewport(&oldVp);
        oldRt->Release();
        return draw();
    }
    if (plan.clearPool) g_pauseHeadPoolClearFrame=frameClearKey;
    const HRESULT drawHr=draw();
    if (pausehead::CaptureAfterDrawAllowed(bindStep,SUCCEEDED(drawHr))) {
        CaptureVrMenuTarget(dev);
        if (g_vrCaptureOwner.CanProvide(VrMenuStateToken()) && g_vrMenuTexture==g_pauseHeadPoolTex) {
            InterlockedIncrement(&g_vrCounters.pauseMenuRedirectDraws);
            InterlockedIncrement(&g_pauseModeRedirects);
            InterlockedExchange(&g_pauseCaptureOk, 1);
        } else {
            InterlockedExchange(&g_pauseCaptureOk, 0);
        }
    }
    g_origSetRenderTarget(dev,0,oldRt);
    dev->SetViewport(&oldVp);
    oldRt->Release();
    return drawHr;
}
static IDirect3DSurface9* g_pauseSceneSurf = nullptr;
void CaptureVrMenuTarget(IDirect3DDevice9* dev);
void PauseMenuMode2NoteSource(IDirect3DDevice9* dev) {
    if (!dev || !pausemode::StretchesScene((int)Rd(g_pauseMenuMode))) return;
    IDirect3DSurface9* src = nullptr;
    if (FAILED(dev->GetRenderTarget(0, &src)) || !src) return;
    if (src == g_pauseSceneSurf) { src->Release(); return; }
    if (g_pauseSceneSurf) g_pauseSceneSurf->Release();
    g_pauseSceneSurf = src;
}
void PauseMenuMode2Capture(IDirect3DDevice9* dev) {
    if (!dev || !pausemode::StretchesScene((int)Rd(g_pauseMenuMode))) return;
    if (Rd(g_vrMenuClass) != 3 || !g_pauseSceneSurf) {
        if (Rd(g_vrMenuClass) == 3) InterlockedExchange(&g_pauseCaptureOk, 0);
        return;
    }
    static unsigned capturedFrame = ~0u;
    const unsigned frame = (unsigned)Rd(g_vfxFrame);
    if (capturedFrame == frame) return;
    D3DSURFACE_DESC desc{};
    g_pauseSceneSurf->GetDesc(&desc);
    if (!EnsurePauseHeadPool(dev)) {
        InterlockedExchange(&g_pauseCaptureOk, 0);
        return;
    }
    const HRESULT stretched = dev->StretchRect(g_pauseSceneSurf, nullptr, g_pauseHeadPoolSurf, nullptr, D3DTEXF_LINEAR);
    if (FAILED(stretched)) {
        InterlockedExchange(&g_pauseCaptureOk, 0);
        static LONG said = 0;
        if (InterlockedCompareExchange(&said, 1, 0) == 0)
            Log("pause-mode: mode=2 stretch failed hr=0x%08lX src=%ux%u", stretched, desc.Width, desc.Height);
        return;
    }
    capturedFrame = frame;
    IDirect3DSurface9* old = nullptr;
    dev->GetRenderTarget(0, &old);
    if (g_origSetRenderTarget) g_origSetRenderTarget(dev, 0, g_pauseHeadPoolSurf);
    CaptureVrMenuTarget(dev);
    if (old) {
        if (g_origSetRenderTarget) g_origSetRenderTarget(dev, 0, old);
        old->Release();
    }
    const bool ok = g_vrMenuTexture == g_pauseHeadPoolTex;
    InterlockedExchange(&g_pauseCaptureOk, ok ? 1 : 0);
    InterlockedIncrement(&g_pauseModeStretches);
    static LONG saidOk = 0;
    if (ok && InterlockedCompareExchange(&saidOk, 1, 0) == 0)
        Log("pause-mode: mode=2 src=scene %ux%u stretched to %ux%u present=head both-eyes",
            desc.Width, desc.Height, pausehead::kPoolWidth, pausehead::kPoolHeight);
}

void PauseCanvasRegisterCapture(std::uint64_t token, unsigned bandA, unsigned bandB, unsigned rtW,
                                unsigned rtH) {
    (void)bandA;
    (void)bandB;
    IDirect3DTexture9* pool = pausecanvas::capture::PoolTexture();
    if (!pool || !rtW || !rtH) {
        InterlockedExchange(&g_pauseCaptureOk, 0);
        return;
    }
    if (VrMenuStateToken() != token) {
        InterlockedExchange(&g_pauseCaptureOk, 0);
        return;
    }
    if (g_vrMenuTexture != pool) {
        if (g_vrMenuTexture) g_vrMenuTexture->Release();
        pool->AddRef();
        g_vrMenuTexture = pool;
    }
    g_vrMenuSurface = reinterpret_cast<uintptr_t>(pool);
    g_vrMenuTextureScore = 0.f;
    g_vrMenuPanel.canvas.cropU0 = 0.f;
    g_vrMenuPanel.canvas.cropU1 = 1.f;
    g_vrMenuPanel.canvas.cropV0 = 0.f;
    g_vrMenuPanel.canvas.cropV1 = 1.f;
    g_vrCaptureOwner.Capture(token, g_vrMenuSurface);
    g_vrMenuTextureFrame = g_vrMenuFrame + 1;
    InterlockedExchange(&g_pauseCaptureOk, 1);
    InterlockedIncrement(&g_vrCounters.pauseMenuCaptured);
}

static void ReleasePauseHeadPool() {
    if (g_pauseSceneSurf) { g_pauseSceneSurf->Release(); g_pauseSceneSurf=nullptr; }
    if (g_pauseHeadPoolSurf) { g_pauseHeadPoolSurf->Release(); g_pauseHeadPoolSurf=nullptr; }
    if (g_pauseHeadPoolTex) { g_pauseHeadPoolTex->Release(); g_pauseHeadPoolTex=nullptr; }
    g_pauseHeadPoolClearFrame=~0u;
    pausecanvas::capture::ReleasePool();
}
static uipointer::Hit g_vrUiHit;
static bool g_vrMenuPanelValid=false;
// Kenney's CC0 PSVR2 prompt subset is packed once into a tiny BMP atlas. The atlas is loaded
// lazily on the first menu frame and then stays resident; the pointer path never performs file I/O.
static IDirect3DTexture9* g_vrPromptAtlas=nullptr;
static bool g_vrPromptAtlasAttempted=false;
struct VrPromptVtx { float x,y,z,rhw; DWORD colour; float u,v; };
struct VrMenuVtx { float x,y,z,rhw; DWORD colour; float u,v; };
static bool LoadVrPromptAtlas(IDirect3DDevice9* dev) {
    if(g_vrPromptAtlasAttempted) return g_vrPromptAtlas!=nullptr;
    g_vrPromptAtlasAttempted=true;
    char path[MAX_PATH]={};
    if(!GetModuleFileNameA(nullptr,path,MAX_PATH)) return false;
    char* slash=strrchr(path,'\\'); if(!slash) return false;
    slash[1]=0; strcat_s(path,"vr-assets\\vr_prompt_atlas.bmp");
    HANDLE f=CreateFileA(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(f==INVALID_HANDLE_VALUE) { Log("VR prompt atlas missing: %s",path); return false; }
    DWORD size=GetFileSize(f,nullptr); if(size<sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)||size>4*1024*1024){CloseHandle(f);return false;}
    unsigned char* bytes=(unsigned char*)malloc(size); DWORD got=0;
    bool ok=bytes&&ReadFile(f,bytes,size,&got,nullptr)&&got==size; CloseHandle(f);
    if(!ok){free(bytes);return false;}
    const BITMAPFILEHEADER* fh=(const BITMAPFILEHEADER*)bytes;
    const BITMAPINFOHEADER* ih=(const BITMAPINFOHEADER*)(bytes+sizeof(BITMAPFILEHEADER));
    const unsigned tw=(unsigned)ih->biWidth, th=(unsigned)ih->biHeight;
    if(fh->bfType!=0x4D42||ih->biSize<sizeof(BITMAPINFOHEADER)||ih->biBitCount!=32||ih->biCompression!=BI_RGB||
       th!=(unsigned)kVrPromptAtlasHeight||tw<(unsigned)kVrPromptIconPx||tw>(unsigned)kVrPromptAtlasWidth||
       (tw%(unsigned)kVrPromptIconPx)!=0||fh->bfOffBits>=size){free(bytes);return false;}
    IDirect3DTexture9* tex=nullptr;
    HRESULT hr=g_origCreateTexture ? g_origCreateTexture(dev,tw,th,1,D3DUSAGE_DYNAMIC,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&tex,nullptr) : dev->CreateTexture(tw,th,1,D3DUSAGE_DYNAMIC,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&tex,nullptr);
    if(FAILED(hr)||!tex){free(bytes);Log("VR prompt atlas texture creation failed: 0x%08lX",hr);return false;}
    D3DLOCKED_RECT lock{};
    if(FAILED(tex->LockRect(0,&lock,nullptr,0))){tex->Release();free(bytes);return false;}
    const unsigned char* src=bytes+fh->bfOffBits;
    for(unsigned y=0;y<th;++y) memcpy((unsigned char*)lock.pBits+y*lock.Pitch,src+(th-1-y)*tw*4,tw*4);
    tex->UnlockRect(0); free(bytes); g_vrPromptAtlas=tex;
    Log("VR prompt atlas loaded: %s",path); return true;
}
static void DrawVrPromptIcon(IDirect3DDevice9* dev,float x,float y,float size,float u0,float u1) {
    VrPromptVtx v[4]={{x,y,0,1,0xFFFFFFFF,u0,0},{x+size,y,0,1,0xFFFFFFFF,u1,0},{x,y+size,0,1,0xFFFFFFFF,u0,1},{x+size,y+size,0,1,0xFFFFFFFF,u1,1}};
    g_origDrawPrimUP(dev,D3DPT_TRIANGLESTRIP,2,v,sizeof(VrPromptVtx));
}
static bool PromptIconUv(const char* name,float& u0,float& u1) {
    if(!name) return false;
    for(int i=0;i<kVrPromptIconCount;++i) {
        if(strcmp(kVrPromptIconUv[i].name,name)==0) {
            u0=kVrPromptIconUv[i].u0; u1=kVrPromptIconUv[i].u1; return true;
        }
    }
    return false;
}
static void DrawPromptRole(IDirect3DDevice9* dev,float x,float y,float size,promptroles::Action action) {
    float u0=0.f,u1=0.f;
    if(PromptIconUv(promptroles::IconName(action),u0,u1)) DrawVrPromptIcon(dev,x,y,size,u0,u1);
}
static void DrawVrPromptLegend(IDirect3DDevice9* dev,const D3DSURFACE_DESC& d,bool gameplay) {
    if(!LoadVrPromptAtlas(dev)) return;
    dev->SetTexture(0,g_vrPromptAtlas); dev->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1);
    dev->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE); dev->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE); dev->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_DIFFUSE);
    dev->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_MODULATE); dev->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE); dev->SetTextureStageState(0,D3DTSS_ALPHAARG2,D3DTA_DIFFUSE);
    dev->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_POINT); dev->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_POINT);
    // Normalized lower-middle safe area.  The old .83/.90 pair put the icons almost under the
    // visible panel on the 2684px SBS target; keeping both coordinates normalized makes this
    // independent of the current eye resolution and crop.
    const float eyeW=d.Width*.5f, size=std::max(22.f,d.Height*.035f), y=d.Height*.72f;
    for(int eye=0;eye<2;++eye){
        const float base=eye*eyeW+eyeW*.12f;
        if(gameplay) {
            DrawPromptRole(dev,base,y,size,promptroles::Action::Interact);
            DrawPromptRole(dev,base+eyeW*.22f,y,size,promptroles::Action::Fire);
            DrawPromptRole(dev,base+eyeW*.44f,y,size,promptroles::Action::Aim);
            DrawPromptRole(dev,base+eyeW*.66f,y,size,promptroles::Action::Confirm);
        } else {
            DrawPromptRole(dev,base,y,size,promptroles::Action::Confirm);
            DrawPromptRole(dev,base+eyeW*.25f,y,size,promptroles::Action::Back);
            DrawPromptRole(dev,base+eyeW*.50f,y,size,promptroles::Action::Pause);
        }
    }
    dev->SetTexture(0,nullptr); dev->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE);
}
extern volatile LONG g_contextPromptVisible;
void DrawVrContextPrompt(IDirect3DDevice9* dev) {
    const LONG active=InterlockedExchange(&g_contextPromptVisible,0);
    if(!active || !dev || !VrModeOn() || !Rd(g_dupDraws) || Rd(g_vrUiMenu) || !g_origDrawPrimUP) return;
    static LONG contextIconLogCount=0;
    LONG cil=InterlockedIncrement(&contextIconLogCount);
    if(cil<=16) Log("context prompt ICON DRAW: active=%ld menu=%ld dup=%ld",active,Rd(g_vrUiMenu),Rd(g_dupDraws));
    IDirect3DSurface9* back=nullptr;
    if(FAILED(dev->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back))||!back)return;
    D3DSURFACE_DESC d{}; back->GetDesc(&d); back->Release();
    if(d.Width<8||d.Height<8||!LoadVrPromptAtlas(dev))return;
    if(!g_wristState && FAILED(dev->CreateStateBlock(D3DSBT_ALL,&g_wristState)))return;
    if(FAILED(g_wristState->Capture()))return;
    IDirect3DSurface9* depth=nullptr;
    dev->GetDepthStencilSurface(&depth);
    dev->SetDepthStencilSurface(nullptr);
    if(g_origSetVS)g_origSetVS(dev,nullptr); else dev->SetVertexShader(nullptr);
    dev->SetPixelShader(nullptr);
    dev->SetRenderState(D3DRS_ZENABLE,FALSE);
    dev->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);
    // GFx leaves alpha-test and texture-coordinate state enabled on some
    // gameplay HUD passes.  The prompt atlas contains antialiased alpha, so
    // inheriting that state can discard the entire glyph while the draw call
    // still succeeds.  Establish the complete fixed-function contract here.
    dev->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);
    dev->SetRenderState(D3DRS_FOGENABLE,FALSE);
    dev->SetRenderState(D3DRS_RANGEFOGENABLE,FALSE);
    dev->SetRenderState(D3DRS_DITHERENABLE,FALSE);
    dev->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE,FALSE);
    dev->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
    dev->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
    dev->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
    dev->SetRenderState(D3DRS_LIGHTING,FALSE);
    dev->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);
    dev->SetRenderState(D3DRS_COLORWRITEENABLE,15);
    dev->SetTexture(0,g_vrPromptAtlas);
    dev->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1);
    dev->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE);
    dev->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);
    dev->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_DIFFUSE);
    dev->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_MODULATE);
    dev->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);
    dev->SetTextureStageState(0,D3DTSS_ALPHAARG2,D3DTA_DIFFUSE);
    dev->SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,0);
    dev->SetTextureStageState(0,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_DISABLE);
    dev->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);
    dev->SetTextureStageState(1,D3DTSS_ALPHAOP,D3DTOP_DISABLE);
    dev->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_POINT);
    dev->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_POINT);
    dev->SetSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP);
    dev->SetSamplerState(0,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP);
    const float halfW=(float)d.Width*.5f;
    const float size=std::max(28.f,(float)d.Height*.045f);
    const float y=(float)d.Height*.62f;
    for(int eye=0;eye<2;++eye) {
        const float x=eye*halfW+(halfW-size)*.5f;
        // The geometry hook identifies the gameplay interaction prompt but
        // does not expose the original GFx label.  This path is used by
        // reload/chest/fountain interactions, whose native VR action is the
        // right-hand grip (R1).  Enter-style note pickup has a separate GFx
        // path and is still handled by the label rewrite table.
        DrawPromptRole(dev,x,y,size,promptroles::Action::Interact);
        InterlockedIncrement(&g_vrCounters.psIconDraws);
    }
    dev->SetTexture(0,nullptr);
    g_wristState->Apply();
    dev->SetDepthStencilSurface(depth);
    if(depth) depth->Release();
}
static uipointer::V VrUiRotate(const XrQuaternionf& q,uipointer::V v) {
    const uipointer::V axis{q.x,q.y,q.z};
    const auto cross=[](uipointer::V a,uipointer::V b){return uipointer::V{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};};
    const auto t=cross(axis,v)*2.f;return v+t*q.w+cross(axis,t);
}
static bool VrUiProjectEye(int eye,uipointer::V point,uipointer::ScreenPoint& out) {
    if(eye<0||eye>1||!g_haveEyes||g_targetTanX<=0||g_targetTanY<=0)return false;
    const XrPosef& p=g_eyePose[eye];
    return uipointer::ProjectPoint(point,{p.position.x,p.position.y,p.position.z},
        VrUiRotate(p.orientation,{0,0,-1}),VrUiRotate(p.orientation,{1,0,0}),
        VrUiRotate(p.orientation,{0,1,0}),g_targetTanX,g_targetTanY,out);
}
static void DrawVrMenuLaser(IDirect3DDevice9* dev,const D3DSURFACE_DESC& d) {
    if(!Rd(g_vrMenuLaser)||!Rd(g_vrUiValid)||!g_handPositionValid[1]||!g_origDrawPrimUP)return;
    const uipointer::V A{g_handLastPos[1].x,g_handLastPos[1].y,g_handLastPos[1].z};
    const uipointer::V B=g_vrUiHit.world;
    const float halfW=(float)d.Width*.5f;
    dev->SetTexture(0,nullptr);dev->SetVertexShader(nullptr);dev->SetPixelShader(nullptr);
    dev->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE);
    dev->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);
    dev->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
    dev->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
    const LONG opacity=std::max(0L,std::min(100L,Rd(g_vrMenuLaserOpacityPct)));
    const DWORD col=(DWORD)((opacity*255/100)<<24)|0x00FFFFFF;
    for(int eye=0;eye<2;++eye){
        uipointer::ScreenPoint a,b;if(!VrUiProjectEye(eye,A,a)||!VrUiProjectEye(eye,B,b))continue;
        const float x0=(eye+a.u)*halfW,y0=a.v*(float)d.Height;
        const float x1=(eye+b.u)*halfW,y1=b.v*(float)d.Height;
        float dx=x1-x0,dy=y1-y0,L=sqrtf(dx*dx+dy*dy);if(L<.5f)continue;dx/=L;dy/=L;
        const float w=std::max(1.f,halfW*.0008f*(float)Rd(g_vrMenuLaserWidthPct)/100.f);
        const float px=-dy*w,py=dx*w;LaserVtx v[4]={{x0+px,y0+py,0,1,col},{x1+px,y1+py,0,1,col},{x0-px,y0-py,0,1,col},{x1-px,y1-py,0,1,col}};
        g_origDrawPrimUP(dev,D3DPT_TRIANGLESTRIP,2,v,sizeof(LaserVtx));
    }
}
static uipointer::V PauseVrSettingsWorld(float u, float v) {
    const float x = (u - .5f) * 2.0f * g_vrMenuPanel.plane.halfWidth * g_vrMenuPanel.plane.inset;
    const float y = (.5f - v) * 2.0f * g_vrMenuPanel.plane.halfHeight * g_vrMenuPanel.plane.inset;
    return g_vrMenuPanel.plane.centre + g_vrMenuPanel.plane.right * x + g_vrMenuPanel.plane.up * y;
}
static void DrawPauseVrSettingsRow(IDirect3DDevice9* dev,const D3DSURFACE_DESC& d) {
    const LONG menuClass = Rd(g_vrMenuClass);
    if (!dev || (menuClass != 2 && menuClass != 3) || !g_origDrawPrimUP) return;
    // The native input hook can discover the pause movie a tick before the
    // pointer update has built the head-relative plane.  Do not suppress the
    // settings entry in that window; build the same plane here as a bounded
    // fallback so the row is always visible when a menu is visible.
    if (!g_vrMenuPanelValid) {
        if (!g_haveEyes || g_targetTanX <= 0 || g_targetTanY <= 0) return;
        const auto& q = g_eyePose[0].orientation;
        const uipointer::V head{
            (g_eyePose[0].position.x + g_eyePose[1].position.x) * .5f,
            (g_eyePose[0].position.y + g_eyePose[1].position.y) * .5f,
            (g_eyePose[0].position.z + g_eyePose[1].position.z) * .5f};
        g_vrMenuPanelValid = uipointer::HeadPlane(
            head, VrUiRotate(q, {0,0,-1}), VrUiRotate(q, {1,0,0}),
            VrUiRotate(q, {0,1,0}), Rd(g_vrMenuDistanceMm) / 1000.0f,
            g_targetTanX, g_targetTanY,
            (menuClass == 2 || menuClass == 3)
                ? std::min(0.84f, Rd(g_menuInsetPct) / 100.0f)
                : Rd(g_menuInsetPct) / 100.0f,
            g_vrMenuPanel.plane);
        if (!g_vrMenuPanelValid) return;
    }
    float rowV0 = 0.0f, rowV1 = 0.0f;
    if (!pausevrsettings::Row((int)menuClass, rowV0, rowV1)) return;
    static LONG lastLoggedClass = -1;
    if (lastLoggedClass != menuClass) {
        lastLoggedClass = menuClass;
        Log("VR settings row: visible in %s menu at normalized V %.2f-%.2f",
            menuClass == 2 ? "main" : "pause", rowV0, rowV1);
    }
    const bool hovered = Rd(g_vrUiValid) && pausevrsettings::Hit(
        (int)menuClass, Rd(g_vrUiU) / 1000000.0f, Rd(g_vrUiV) / 1000000.0f);
    const float halfW = d.Width * .5f;
    const int px = std::max(2, (int)d.Height / 420);
    const int textW = 10 * (kGlyphW + 1) * px;
    const int textH = kGlyphH * px;
    D3DRECT text[128];
    for (int eye = 0; eye < 2; ++eye) {
        uipointer::ScreenPoint a,b,c;
        if (!VrUiProjectEye(eye, PauseVrSettingsWorld(pausevrsettings::kU0, rowV0), a) ||
            !VrUiProjectEye(eye, PauseVrSettingsWorld(pausevrsettings::kU1, rowV1), b) ||
            !VrUiProjectEye(eye, PauseVrSettingsWorld(.5f, (rowV0 + rowV1) * .5f), c)) continue;
        const int x0 = (int)lroundf((eye + std::min(a.u,b.u)) * halfW);
        const int x1 = (int)lroundf((eye + std::max(a.u,b.u)) * halfW);
        const int y0 = (int)lroundf(std::min(a.v,b.v) * d.Height);
        const int y1 = (int)lroundf(std::max(a.v,b.v) * d.Height);
        const D3DRECT box = { x0, y0, x1, y1 };
        ClearRects(dev, &box, 1, hovered ? 0xD0507040 : 0xB0081218);
        const int cx = (int)lroundf((eye + c.u) * halfW);
        const int cy = (int)lroundf(c.v * d.Height);
        const int n = TextRects(text, 0, (int)(sizeof(text) / sizeof(text[0])),
                                cx - textW / 2, cy - textH / 2, px, "VR SETTINGS");
        ClearRects(dev, text, n, hovered ? 0xFFFFFFFF : 0xFF50FF90);
    }
}
void UpdateVrUiPointer(float,bool available) {
    // The in-game GFx menu and the mod-owned VR Settings panel share one
    // controller ray.  The panel is deliberately not marked as g_vrUiMenu
    // (that flag suppresses native menu capture), so include it explicitly or
    // the panel can only be driven by the stick and never receives laser hits.
    const bool panel=Rd(g_panelOpen)!=0;
    // The mod-owned panel is usable even while the native GFx hook is not ready.
    // Requiring g_vrUiNativeReady here made the laser disappear exactly when the
    // user opened VR Settings during the early menu frames.
    const bool menu=Rd(g_vrUiEnabled)&&(panel || Rd(g_vrUiNativeReady))&&(Rd(g_vrUiMenu)||panel);
    if(menu&&Rd(g_vrUiDesktopProbe))return; // Explicit debugger-controlled test coordinates; never staged for headset use.
    if(!available || !menu || !g_handPoseValid[1] || !g_handPositionValid[1] || !g_haveEyes || g_targetTanX<=0 || g_targetTanY<=0) {
        g_vrMenuPanelValid=false;
        InterlockedExchange(&g_vrUiValid,0);InterlockedExchange(&g_vrUiDown,0);
        if(!available)for(auto& gate:g_vrUiTrigger)gate.Update(menu?uipointer::Context::Menu:uipointer::Context::Gameplay,false,false,false);
        return;
    }
    const auto& q=g_eyePose[0].orientation;
    uipointer::V head{(g_eyePose[0].position.x+g_eyePose[1].position.x)*.5f,(g_eyePose[0].position.y+g_eyePose[1].position.y)*.5f,(g_eyePose[0].position.z+g_eyePose[1].position.z)*.5f};
    uipointer::Hit hit;
    const auto& pos=g_handLastPos[1];
    float inset=Rd(g_menuInsetPct)/100.f;
    const LONG menuClass=Rd(g_vrMenuClass);
    if ((menuClass==2 || menuClass==3) && inset>0.84f) inset=0.84f;
    const float distance=Rd(g_vrMenuDistanceMm)/1000.0f;
    g_vrMenuPanelValid=uipointer::HeadPlane(head,VrUiRotate(q,{0,0,-1}),VrUiRotate(q,{1,0,0}),VrUiRotate(q,{0,1,0}),distance,g_targetTanX,g_targetTanY,inset,g_vrMenuPanel.plane);
    bool valid=g_vrMenuPanelValid && g_vrMenuPanel.Ray({pos.x,pos.y,pos.z},VrUiRotate(g_handAimOrientation[1],{0,0,-1}),hit);
    if(valid){
        InterlockedIncrement(&g_vrCounters.vrPointerUpdates);
        g_vrUiHit=hit;InterlockedExchange(&g_vrUiU,(LONG)lroundf(hit.u*1000000));InterlockedExchange(&g_vrUiV,(LONG)lroundf(hit.v*1000000));
        for(int eye=0;eye<2;++eye){uipointer::ScreenPoint c;if(VrUiProjectEye(eye,g_vrMenuPanel.plane.centre,c))InterlockedExchange(&g_vrUiEyeCentreU[eye],(LONG)lroundf(c.u*1000000));}
    }
    else InterlockedExchange(&g_vrUiDown,0);
    InterlockedExchange(&g_vrUiValid,valid?1:0);
}
void SyncVrMenuCaptureState() {
    const auto token=VrMenuStateToken();
    if(g_vrCaptureOwner.state==token)return;
    if(g_vrMenuTexture)InterlockedIncrement(&g_vrStaleRejected);
    Log("MenuCapture transition old=%llu source=%p texture=%p new=%llu staleRejected=%ld",
        g_vrCaptureOwner.state,(void*)g_vrCaptureOwner.source,(void*)g_vrMenuTexture,token,Rd(g_vrStaleRejected));
    if(g_vrMenuTexture){g_vrMenuTexture->Release();g_vrMenuTexture=nullptr;}
    for(unsigned i=0;i<g_vrMenuSourceCount;++i)if(g_vrMenuSources[i])g_vrMenuSources[i]->Release();
    memset(g_vrMenuSources,0,sizeof(g_vrMenuSources));g_vrMenuSourceCount=0;
    // Fix-H/menu: the stage-bound flags below belong to these exact textures. Without
    // clearing, a transition leaves stale bits that misattribute the next menu's draws
    // until every stage rebinds. Transition-only, so the per-draw cost is untouched.
    for(int sb=0;sb<4;++sb)InterlockedExchange(&g_vrMenuStageBound[sb],0);
    g_vrMenuTextureScore=1e9f;g_vrMenuSurface=0;
    InterlockedExchange(&g_vrMenuSourceBound,0);
    g_vrCaptureOwner.Invalidate(token);
}
void CaptureVrMenuTarget(IDirect3DDevice9* dev) {
    SyncVrMenuCaptureState();
    const auto captureState=VrMenuStateToken();
    if(!Rd(g_vrUiEnabled)||!Rd(g_vrUiMenu)||!Rd(g_vrUiNativeReady)||g_boundShaderLayout.shadow)return;
    // GFx menu surfaces are not guaranteed to carry the identity c5 orthographic constants.
    // Requiring that matrix silently missed the live menu and left only its lower-right diegetic
    // monitor visible. Read c5 for diagnostics, but identify the target by bounded colour format
    // and dimensions below so it can be copied to the centered head-relative overlay.
    // Private GFx targets frequently draw through a fixed-function or a shader that does not
    // expose the scene c5 matrix.  The render-target descriptor is the reliable discriminator;
    // keep the read only as optional diagnostics and never make capture depend on it.
    float c[16];(void)dev->GetVertexShaderConstantF(5,c,4);
    IDirect3DSurface9* surface=nullptr;if(FAILED(dev->GetRenderTarget(0,&surface))||!surface)return;
    D3DSURFACE_DESC desc{};surface->GetDesc(&desc);
    if(desc.Width<640||desc.Height<360||desc.Width>2048||desc.Height>1536||
       (desc.Format!=D3DFMT_A8R8G8B8&&desc.Format!=D3DFMT_X8R8G8B8)){surface->Release();return;}
    IDirect3DTexture9* texture=nullptr;
    if(SUCCEEDED(surface->GetContainer(__uuidof(IDirect3DTexture9),(void**)&texture))&&texture){
        bool known=false;
        for(unsigned i=0;i<g_vrMenuSourceCount;++i) if(g_vrMenuSources[i]==texture){known=true;break;}
        const bool retained=!known && g_vrMenuSourceCount<8;
        if(retained) {
            g_vrMenuSources[g_vrMenuSourceCount++]=texture;
            InterlockedIncrement(&g_vrMenuCaptured);
            if(Rd(g_vrMenuClass)==1)InterlockedIncrement(&g_vrCounters.bootMenuCaptured);
            if(Rd(g_vrMenuClass)==2)InterlockedIncrement(&g_vrCounters.mainMenuCaptured);
            if(Rd(g_vrMenuClass)==3)InterlockedIncrement(&g_vrCounters.pauseMenuCaptured);
            Log("VR menu sink: private GFx candidate %ux%u registered; all matching scene draws will be suppressed",desc.Width,desc.Height);
        }
        // Prefer the candidate whose aspect is closest to the native GFx canvas.  The source list
        // is retained as well because the physical menu can be submitted through more than one
        // monitor/resolve target; suppressing only the selected texture leaves a second menu alive.
        const float ratio=(float)desc.Width/(float)desc.Height;
        const float score=fabsf(ratio-(16.0f/9.0f)) + (desc.Width<960 ? .15f : 0.0f);
        if((!g_vrMenuTexture || score<g_vrMenuTextureScore) && (known || retained)){
            if(g_vrMenuTexture)g_vrMenuTexture->Release();
            texture->AddRef();
            g_vrMenuTexture=texture;g_vrMenuSurface=(uintptr_t)surface;g_vrMenuTextureScore=score;
            g_vrCaptureOwner.Capture(captureState,(uintptr_t)surface);
        }
        if(known || !retained) texture->Release(); // retained entries own the GetContainer reference
        g_vrMenuTextureFrame=g_vrMenuFrame+1;
    }
    surface->Release();
}
void ReleaseVrMenuResources(){
    g_vrCaptureOwner.Invalidate(VrMenuStateToken());
    ReleasePauseHeadPool();
    ReleaseFpCasterCache();
    if(g_vrMenuTexture){g_vrMenuTexture->Release();g_vrMenuTexture=nullptr;}
    for(unsigned i=0;i<g_vrMenuSourceCount;++i)if(g_vrMenuSources[i])g_vrMenuSources[i]->Release();
    memset(g_vrMenuSources,0,sizeof(g_vrMenuSources));g_vrMenuSourceCount=0;
    if(g_vrPromptAtlas){g_vrPromptAtlas->Release();g_vrPromptAtlas=nullptr;}
    g_vrPromptAtlasAttempted=false;g_vrMenuSurface=0;g_vrMenuTextureFrame=~0u;g_vrMenuTextureScore=1e9f;InterlockedExchange(&g_vrMenuSourceBound,0);
}
// Render the captured GFx menu texture as a single head-relative quad at VrMenuDistanceMm.
// Also draws the laser pointer and prompt legend.
void DrawVrMenuPointer(IDirect3DDevice9* dev) {
    ++g_vrMenuFrame;
    SyncVrMenuCaptureState();
    if (pausemode::StretchesScene((int)Rd(g_pauseMenuMode))) PauseMenuMode2Capture(dev);
    MenuDrawTraceFrame();
    const bool nativeMenu = Rd(g_vrUiEnabled) && VrModeOn() && Rd(g_dupDraws) && Rd(g_vrUiNativeReady) &&
                            Rd(g_vrUiMenu) && Rd(g_vrMenuOverlay) && !Rd(g_panelOpen);
    const bool menuRow = Rd(g_vrUiEnabled) && VrModeOn() && Rd(g_dupDraws) && Rd(g_vrUiNativeReady) &&
                         (Rd(g_vrMenuClass) == 2 || Rd(g_vrMenuClass) == 3);
    const bool panel = Rd(g_vrUiEnabled) && VrModeOn() && Rd(g_dupDraws) && Rd(g_panelOpen);
    const bool menu = nativeMenu || menuRow || panel;
    if (!menu || !g_origDrawPrimUP || !g_origSetVS || !g_origSetRenderTarget) return;

    IDirect3DSurface9 *back = nullptr, *old = nullptr, *depth = nullptr;
    if (FAILED(dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &back)) || !back) return;
    D3DSURFACE_DESC d{}; back->GetDesc(&d);
    if (d.Width < 4 || d.Height < 4) { back->Release(); return; }

    dev->GetRenderTarget(0, &old); dev->GetDepthStencilSurface(&depth);
    if (!g_wristState && FAILED(dev->CreateStateBlock(D3DSBT_ALL, &g_wristState))) { if (old) old->Release(); if (depth) depth->Release(); back->Release(); return; }
    if (FAILED(g_wristState->Capture())) { if (old) old->Release(); if (depth) depth->Release(); back->Release(); return; }

    dev->SetDepthStencilSurface(nullptr);
    g_origSetRenderTarget(dev, 0, back);
    D3DVIEWPORT9 vp{0, 0, d.Width, d.Height, 0, 1}; dev->SetViewport(&vp);
    g_origSetVS(dev, nullptr); dev->SetPixelShader(nullptr);
    dev->SetRenderState(D3DRS_ZENABLE, FALSE);
    dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    dev->SetRenderState(D3DRS_LIGHTING, FALSE);
    dev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
    dev->SetRenderState(D3DRS_COLORWRITEENABLE, 15);
    dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

    // ---- RENDER MENU QUAD ----
    // Class 3 uses the pause pool only for modes that opted into a head capture.
    // Mode 0/3 must not keep showing a stale pool from a previous selection.
    const int pauseMode = pausemode::Clamp((int)Rd(g_pauseMenuMode));
    const bool class3 = Rd(g_vrMenuClass) == 3;
    IDirect3DTexture9* const canvasPool = pausecanvas::capture::PoolTexture();
    const bool pauseHead = class3 && pausemode::ShowsHeadCapture(pauseMode) && Rd(g_pauseCaptureOk) &&
                           g_vrMenuTexture &&
                           (pauseMode == pausemode::CanvasCapture
                                ? (canvasPool && g_vrMenuTexture == canvasPool)
                                : (g_pauseHeadPoolTex && g_vrMenuTexture == g_pauseHeadPoolTex));
    const bool showCaptured = nativeMenu && g_vrMenuTexture && g_vrMenuPanelValid &&
                              g_vrCaptureOwner.CanProvide(VrMenuStateToken()) &&
                              (!class3 || pauseHead);
    if (showCaptured) {
        for(int eye=0;eye<2;++eye) {
            VrMenuVtx v[4]; bool visible=true;
            for(unsigned k=0;k<4;++k) {
                uipointer::ScreenPoint p;
                if(!VrUiProjectEye(eye,g_vrMenuPanel.Corner(k),p)){visible=false;break;}
                const auto& crop=g_vrMenuPanel.canvas;
                v[k]={(eye+p.u)*d.Width*.5f,p.v*d.Height,0,1,0xFFFFFFFF,
                      (k&1)?crop.cropU1:crop.cropU0,(k&2)?crop.cropV1:crop.cropV0};
            }
            if(!visible)continue;
            InterlockedExchange(&g_vrMenuProviderDrawing,1);
            if(g_origSetTexture)g_origSetTexture(dev,0,g_vrMenuTexture);
            else dev->SetTexture(0,g_vrMenuTexture);
            dev->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1);
            g_origDrawPrimUP(dev,D3DPT_TRIANGLESTRIP,2,v,sizeof(VrMenuVtx));
            if(Rd(g_vrMenuClass)==1)InterlockedIncrement(&g_vrCounters.bootMenuProvided);
            if(Rd(g_vrMenuClass)==2)InterlockedIncrement(&g_vrCounters.mainMenuProvided);
            if(Rd(g_vrMenuClass)==3)InterlockedIncrement(&g_vrCounters.pauseMenuProvided);
            if(g_origSetTexture)g_origSetTexture(dev,0,nullptr);
            else dev->SetTexture(0,nullptr);
            InterlockedExchange(&g_vrMenuProviderDrawing,0);
        }
    }

    // The synthetic row stays visible even when the controller ray is currently
    // outside the menu plane; only hover/click feedback depends on a valid hit.
    if (menuRow && !panel) DrawPauseVrSettingsRow(dev, d);

    // ---- LASER + HIT MARKER ----
    if (menu && Rd(g_vrUiValid)) {
        DrawVrMenuLaser(dev, d);
        D3DRECT outline[6], dot[6]; int n = 0; const int radius = std::max(3, (int)d.Height / 420);
        for (int eye = 0; eye < 2; ++eye) {
            uipointer::ScreenPoint p; if (!VrUiProjectEye(eye, g_vrUiHit.world, p)) continue;
            const int x = (int)lroundf((eye + p.u) * d.Width * .5f), y = (int)lroundf(p.v * d.Height);
            const int rows[3] = { radius - 1, radius, radius - 1 };
            for (int j = 0; j < 3; ++j) { const int yy = y + j - 1, rr = rows[j]; outline[n] = { x - rr - 1, yy - 1, x + rr + 2, yy + 2 }; dot[n] = { x - rr, yy, x + rr + 1, yy + 1 }; ++n; }
        }
        ClearRects(dev, outline, n, 0xFF000000); ClearRects(dev, dot, n, Rd(g_vrUiDown) ? 0xFF50FF80 : 0xFFFFFFFF);
    }

    // Menu legends are intentionally omitted.  PS icons are reserved for
    // contextual gameplay prompts (for example Enter -> Cross when picking up
    // a note) so the menu remains the game's native layout.

    if (old) g_origSetRenderTarget(dev, 0, old);
    dev->SetDepthStencilSurface(depth);
    g_wristState->Apply();
    if (old) old->Release(); if (depth) depth->Release(); back->Release();
}
