#pragma once
// Included after the D3D state/overlay helpers. No per-frame allocation or scan.
#include "menu_draw_trace.h"
#include "prompt_roles.h"
#include "prompt_atlas_generated.h"
static uintptr_t g_vrMenuSurface=0;
static unsigned g_vrMenuFrame=0,g_vrMenuTextureFrame=0;
static uipointer::Hit g_vrUiHit;
static bool g_vrMenuPanelValid=false;
// Kenney's CC0 PSVR2 prompt subset is packed once into a tiny BMP atlas. The atlas is loaded
// lazily on the first menu frame and then stays resident; the pointer path never performs file I/O.
static IDirect3DTexture9* g_vrPromptAtlas=nullptr;
static bool g_vrPromptAtlasAttempted=false;
struct VrPromptVtx { float x,y,z,rhw; DWORD colour; float u,v; };
struct VrMenuVtx { float x,y,z,rhw; DWORD colour; float u,v; };
static bool VrUiProjectEye(int eye,uipointer::V point,uipointer::ScreenPoint& out);
static frontmenu::Resources g_frontMenuResources;
static bool ComposeCleanFrontend(IDirect3DDevice9* dev,IDirect3DSurface9* back,
                                const D3DSURFACE_DESC& desc,bool nativeMenu) {
    const frontmenu::Signals signals{Rd(g_frontMenuCleanBackdrop)!=0,VrModeOn(),Rd(g_dupDraws)!=0,
        Rd(g_vrUiMenu)!=0,nativeMenu,Rd(g_panelOpen)!=0,Rd(g_modalSaveDialog)!=0,
        g_vrCaptureOwner.CanProvide(VrMenuStateToken()),g_vrMenuPanelValid,
        g_sceneMode.Current()==vrscene::Mode::MenuFlat,
        g_sceneMode.Current()==vrscene::Mode::VideoFlat,(int)Rd(g_vrMenuClass)};
    if(!frontmenu::Eligible(signals)||!g_vrMenuTexture||!g_origSetRenderTarget||!g_origSetVS||!g_origDrawPrimUP)return false;
    if(desc.Width<8||desc.Height<8||(desc.Width&1))return false;
    const auto drawnToken=VrMenuStateToken();
    frontmenu::Vertex vertices[2][4];
    for(int eye=0;eye<2;++eye)for(unsigned k=0;k<4;++k){
        uipointer::ScreenPoint p;
        if(!VrUiProjectEye(eye,g_vrMenuPanel.Corner(k),p)||!std::isfinite(p.u)||!std::isfinite(p.v))return false;
        const auto& crop=g_vrMenuPanel.canvas;
        vertices[eye][k]={(eye+p.u)*desc.Width*.5f,p.v*desc.Height,0,1,0xFFFFFFFF,
            (k&1)?crop.cropU1:crop.cropU0,(k&2)?crop.cropV1:crop.cropV0};
    }
    frontmenu::SurfacePass pass(dev,back,g_vrMenuTexture,g_frontMenuResources,desc,vertices,
        g_origSetRenderTarget,g_origSetVS,g_origSetTexture,g_origDrawPrimUP);
    InterlockedExchange(&g_vrMenuProviderDrawing,1);
    const auto result=frontmenu::Render(pass);
    InterlockedExchange(&g_vrMenuProviderDrawing,0);
    if(!result.stateRestored||!result.pixelsRestored)g_frontMenuResources.fault=true;
    if(result.composed)g_frontMenuCleanToken=drawnToken;
    static unsigned notes=0;static unsigned long long notedToken=0;
    if(notes<16 && (notedToken!=VrMenuStateToken()||!result.composed)){
        notedToken=VrMenuStateToken();++notes;
        Log("frontend-clean: token=%llu composed=%d stateRestore=%d pixelRestore=%d source=%p back=%ux%u perEye=1",
            notedToken,result.composed?1:0,result.stateRestored?1:0,result.pixelsRestored?1:0,
            (void*)g_vrMenuTexture,desc.Width,desc.Height);
    }
    return result.composed;
}
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
    LONG pointerU=500000,pointerV=500000;ReadVrUiPair(pointerU,pointerV);
    const uipointer::V B=uipointer::PointAtCanvasUv(g_vrMenuPanel.plane,pointerU/1000000.0f,pointerV/1000000.0f);
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
static void DrawVrMenuReticle(IDirect3DDevice9* dev,const D3DSURFACE_DESC& d,
                              uipointer::V point,bool pressed) {
    if(!dev||!g_origDrawPrimUP||!g_haveEyes||d.Width<8||d.Height<8)return;
    static constexpr int kSegments=32;
    DWORD oldTextureOp=0,oldAlphaOp=0;
    dev->GetTextureStageState(0,D3DTSS_COLOROP,&oldTextureOp);
    dev->GetTextureStageState(0,D3DTSS_ALPHAOP,&oldAlphaOp);
    if(g_origSetTexture)g_origSetTexture(dev,0,nullptr);else dev->SetTexture(0,nullptr);
    dev->SetVertexShader(nullptr);dev->SetPixelShader(nullptr);
    dev->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE);
    dev->SetRenderState(D3DRS_ZENABLE,FALSE);dev->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);
    dev->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);
    dev->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
    dev->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
    dev->SetRenderState(D3DRS_SCISSORTESTENABLE,TRUE);
    dev->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);
    dev->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);
    dev->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);
    dev->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_DIFFUSE);
    const menupointer::Style style=menupointer::AtTime(GetTickCount64(),pressed,(float)d.Height);
    const DWORD ringColour=pressed?0xEE56FF9Au:0xE650E8F2u;
    const DWORD innerColour=pressed?0xD8FFFFFFu:0xD5FFFFFFu;
    for(int eye=0;eye<2;++eye){
        uipointer::ScreenPoint projected;if(!VrUiProjectEye(eye,point,projected))continue;
        const float cx=(eye+projected.u)*(float)d.Width*.5f,cy=projected.v*(float)d.Height;
        const LONG half=(LONG)d.Width/2;const RECT clip{eye?half:0,0,eye?(LONG)d.Width:half,(LONG)d.Height};
        dev->SetScissorRect(&clip);
        auto ring=[&](float radius,float thickness,DWORD colour){
            LaserVtx vertices[2*(kSegments+1)];
            for(int i=0;i<=kSegments;++i){
                const float a=(float)i*(6.28318530718f/(float)kSegments);
                const float co=cosf(a),si=sinf(a);
                vertices[2*i]={cx+co*radius,cy+si*radius,0,1,colour};
                vertices[2*i+1]={cx+co*(radius-thickness),cy+si*(radius-thickness),0,1,colour};
            }
            g_origDrawPrimUP(dev,D3DPT_TRIANGLESTRIP,2*kSegments,vertices,sizeof(LaserVtx));
        };
        ring(style.outerRadius,style.ringThickness,ringColour);
        ring(style.outerRadius*.67f,1.35f,innerColour);
        LaserVtx core[kSegments+2]{};
        core[0]={cx,cy,0,1,0xEBFFFFFFu};
        for(int i=0;i<=kSegments;++i){
            const float a=(float)i*(6.28318530718f/(float)kSegments);
            core[i+1]={cx+cosf(a)*style.coreRadius,cy+sinf(a)*style.coreRadius,0,1,0xEBFFFFFFu};
        }
        g_origDrawPrimUP(dev,D3DPT_TRIANGLEFAN,kSegments,core,sizeof(LaserVtx));
    }
    dev->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);
    dev->SetTextureStageState(0,D3DTSS_COLOROP,oldTextureOp);
    dev->SetTextureStageState(0,D3DTSS_ALPHAOP,oldAlphaOp);
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
            VrUiRotate(q, {0,1,0}), FrontMenuDistance((int)menuClass,false,(int)Rd(g_vrMenuDistanceMm),(int)Rd(g_frontMenuDistanceMm)),
            g_targetTanX, g_targetTanY,
            FrontMenuInset((int)menuClass,false,(menuClass == 2 || menuClass == 3)
                ? std::min(0.84f, Rd(g_menuInsetPct) / 100.0f)
                : Rd(g_menuInsetPct) / 100.0f,(int)Rd(g_frontMenuScalePct)),
            g_vrMenuPanel.plane);
        if (!g_vrMenuPanelValid) return;
        g_vrMenuPanel.plane.centre = g_vrMenuPanel.plane.centre +
            g_vrMenuPanel.plane.up * (MenuPlacementClipY((int)menuClass) * g_vrMenuPanel.plane.halfHeight);
        if (menuClass == 3) uipointer::CompactNativePause(g_vrMenuPanel.plane);
    }
    float rowV0 = 0.0f, rowV1 = 0.0f;
    if (!pausevrsettings::Row((int)menuClass, rowV0, rowV1)) return;
    static LONG lastLoggedClass = -1;
    if (lastLoggedClass != menuClass) {
        lastLoggedClass = menuClass;
        Log("VR settings row: visible in %s menu at normalized V %.2f-%.2f",
            menuClass == 2 ? "main" : "pause", rowV0, rowV1);
    }
    LONG hoverU=500000,hoverV=500000;ReadVrUiPair(hoverU,hoverV);
    const bool hovered = Rd(g_vrUiValid) && pausevrsettings::Hit(
        (int)menuClass, hoverU / 1000000.0f, hoverV / 1000000.0f);
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
    static unsigned long long pointerToken=~0ull;
    const unsigned long long token=VrMenuStateToken();
    if(token!=pointerToken){pointerToken=token;PublishVrUiPair(500000,500000);InterlockedExchange(&g_vrUiEverHit,0);}
    if(menu&&Rd(g_vrUiDesktopProbe))return; // Explicit debugger-controlled test coordinates; never staged for headset use.
    if(!available || !menu || !g_handPoseValid[1] || !g_handPositionValid[1] || !g_haveEyes || g_targetTanX<=0 || g_targetTanY<=0) {
        if(!available || !menu)g_vrMenuPanelValid=false;
        InterlockedExchange(&g_vrUiValid,0);InterlockedExchange(&g_vrUiDown,0);
        if(!menu&&Rd(g_vrUiEverHit))InterlockedExchange(&g_vrUiEverHit,0);
        if(!available)for(auto& gate:g_vrUiTrigger)gate.Update(menu?uipointer::Context::Menu:uipointer::Context::Gameplay,false,false,false);
        return;
    }
    const auto& q=g_eyePose[0].orientation;
    uipointer::V head{(g_eyePose[0].position.x+g_eyePose[1].position.x)*.5f,(g_eyePose[0].position.y+g_eyePose[1].position.y)*.5f,(g_eyePose[0].position.z+g_eyePose[1].position.z)*.5f};
    uipointer::Hit hit;
    const auto& pos=g_handLastPos[1];
    float inset=Rd(g_menuInsetPct)/100.f;
    const LONG menuClass=Rd(g_vrMenuClass);
    if (!panel && menuClass == 1) inset = ClassBackbufferScale(1, true, (int)Rd(g_menuInsetPct));
    else if ((menuClass==2 || menuClass==3) && inset>0.84f) inset=0.84f;
    inset=FrontMenuInset((int)menuClass,panel,inset,(int)Rd(g_frontMenuScalePct));
    const float distance=FrontMenuDistance((int)menuClass,panel,(int)Rd(g_vrMenuDistanceMm),(int)Rd(g_frontMenuDistanceMm));
    g_vrMenuPanelValid=uipointer::HeadPlane(head,VrUiRotate(q,{0,0,-1}),VrUiRotate(q,{1,0,0}),VrUiRotate(q,{0,1,0}),distance,g_targetTanX,g_targetTanY,inset,g_vrMenuPanel.plane);
    if (!panel && g_vrMenuPanelValid)
        g_vrMenuPanel.plane.centre = g_vrMenuPanel.plane.centre +
            g_vrMenuPanel.plane.up * (MenuPlacementClipY((int)menuClass) * g_vrMenuPanel.plane.halfHeight);
    if (menuClass == 3 && !panel && g_vrMenuPanelValid) {
        uipointer::CompactNativePause(g_vrMenuPanel.plane);
        g_vrMenuPanel.canvas = uipointer::CanvasMap{};
    }
    bool valid=g_vrMenuPanelValid && g_vrMenuPanel.Ray({pos.x,pos.y,pos.z},VrUiRotate(g_handAimOrientation[1],{0,0,-1}),hit);
    if(valid){
        InterlockedIncrement(&g_vrCounters.vrPointerUpdates);
        g_vrUiHit=hit;PublishVrUiPair((LONG)lroundf(hit.u*1000000),(LONG)lroundf(hit.v*1000000));
        InterlockedExchange(&g_vrUiEverHit,1);
        for(int eye=0;eye<2;++eye){uipointer::ScreenPoint c;if(VrUiProjectEye(eye,g_vrMenuPanel.plane.centre,c))InterlockedExchange(&g_vrUiEyeCentreU[eye],(LONG)lroundf(c.u*1000000));}
    }
    else InterlockedExchange(&g_vrUiDown,0);
    InterlockedExchange(&g_vrUiValid,valid?1:0);
}
static void ReleaseModalParentSources() {
    for(unsigned i=0;i<g_vrModalParentSourceCount;++i)
        if(g_vrModalParentSources[i])g_vrModalParentSources[i]->Release();
    memset(g_vrModalParentSources,0,sizeof(g_vrModalParentSources));
    g_vrModalParentSourceCount=0;
    for(int stage=0;stage<4;++stage)InterlockedExchange(&g_vrModalParentStageBound[stage],0);
}
static void PreserveModalParentSources() {
    if(g_vrModalParentSourceCount||!Rd(g_modalSlotsCapturePending)||
       !Rd(g_modalSaveDialog)||!Rd(g_modalSlotsActive))return;
    InterlockedExchange(&g_modalSlotsCapturePending,0);
    for(unsigned i=0;i<g_vrMenuSourceCount&&g_vrModalParentSourceCount<8;++i){
        IDirect3DTexture9* texture=g_vrMenuSources[i];
        if(!texture)continue;
        texture->AddRef();g_vrModalParentSources[g_vrModalParentSourceCount++]=texture;
    }
    for(int stage=0;stage<4;++stage)
        if(Rd(g_vrMenuStageBound[stage]))InterlockedExchange(&g_vrModalParentStageBound[stage],1);
    if(g_vrModalParentSourceCount)
        Log("menu modal: retained %u exact Slots source textures for suppression until Dialog closes",g_vrModalParentSourceCount);
    else
        Log("menu modal: Slots source list empty at focus transition; no parent suppression until exact source capture");
}
void SyncVrMenuCaptureState() {
    // This function is called from draw hooks. Retained COM sources are released
    // only once when the modal lifetime ends; do not pay interlocked writes on
    // every scene draw while no modal source is retained.
    // Preserve the exact Slots source before a token transition clears the source
    // list. Input-thread focus can change before the render thread consumes the
    // explicit release request, so relying only on ReleaseVrMenuResources(true)
    // loses the parent RTT when another draw hook reaches Sync first.
    if(Rd(g_modalSlotsCapturePending)&&Rd(g_modalSaveDialog)&&Rd(g_modalSlotsActive))
        PreserveModalParentSources();
    if((!Rd(g_modalSaveDialog)||!Rd(g_modalSlotsActive))&&g_vrModalParentSourceCount)
        ReleaseModalParentSources();
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
    // Pause now has an identity-proven native RTT; generic size candidates
    // are never allowed to select or suppress its source.
    if (Rd(g_vrMenuClass) == 3) return;
    const bool exactModalDialog=modalmenu::CaptureExactDialog(
        Rd(g_modalSaveDialog)!=0,Rd(g_modalSlotsActive)!=0,Rd(g_vrModalDialogSurface)!=0);
    if (Rd(g_modalSaveDialog) && !exactModalDialog) return;
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
    if(exactModalDialog&&surface!=(IDirect3DSurface9*)(uintptr_t)(DWORD)Rd(g_vrModalDialogSurface)){
        surface->Release();return;
    }
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
            Log("VR menu sink: private GFx candidate %ux%u registered rt=%p texture=%p class=%ld token=%llu exactDialog=%d; all matching scene draws will be suppressed",desc.Width,desc.Height,(void*)surface,(void*)texture,Rd(g_vrMenuClass),captureState,exactModalDialog?1:0);
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
void ReleaseVrMenuResources(bool menuFocusTransition){
    if(menuFocusTransition&&Rd(g_modalSlotsCapturePending)&&Rd(g_modalSaveDialog)&&Rd(g_modalSlotsActive))
        PreserveModalParentSources();
    else if(!Rd(g_modalSaveDialog)||!Rd(g_modalSlotsActive))ReleaseModalParentSources();
    g_frontMenuResources.Release();g_frontMenuCleanFrame=false;
    g_vrCaptureOwner.Invalidate(VrMenuStateToken());
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
    if (InterlockedExchange(&g_vrMenuReleaseRequested, 0)) ReleaseVrMenuResources(true);
    ++g_vrMenuFrame;
    SyncVrMenuCaptureState();
    // One stable boot and main proof image per process, actual XR source only.
    static LONG placementClass = -1;
    static unsigned placementFrames = 0, placementCaptured = 0;
    const LONG placementNow = Rd(g_vrMenuClass);
    if (placementClass != placementNow) { placementClass=placementNow; placementFrames=0; }
    if ((placementNow == 1 || placementNow == 2) && ++placementFrames == 30 &&
        !(placementCaptured & (1u << placementNow))) {
        placementCaptured |= 1u << placementNow;
        Log("menu-placement proof class=%ld inset=%.4f clipY=%.2f distance=%.3fm", placementNow,
            FrontMenuInset((int)placementNow,false,ClassBackbufferScale((int)placementNow,true,(int)Rd(g_menuInsetPct)),(int)Rd(g_frontMenuScalePct)),
            MenuPlacementClipY((int)placementNow),FrontMenuDistance((int)placementNow,false,(int)Rd(g_vrMenuDistanceMm),(int)Rd(g_frontMenuDistanceMm)));
        InterlockedExchange(&g_pauseCaptureRequested,1);
    }
    const bool dedicatedPause = PauseRttProbe::Ready();
    static bool sawPause = false, capturedReturn = false;
    if (dedicatedPause) sawPause = true;
    if (sawPause && !capturedReturn && Rd(g_vrMenuClass) == 2) {
        capturedReturn = true;
        InterlockedExchange(&g_pauseCaptureRequested, 1);
    }
    if (dedicatedPause) {
        if (g_vrMenuTexture != PauseRttProbe::contentTexture) {
            if (g_vrMenuTexture) g_vrMenuTexture->Release();
            g_vrMenuTexture = PauseRttProbe::contentTexture; g_vrMenuTexture->AddRef();
            g_vrMenuSurface = (uintptr_t)PauseRttProbe::contentSurface;
        }
        g_vrCaptureOwner.Capture(VrMenuStateToken(), g_vrMenuSurface);
    }
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

    // Present is outside BeginScene. Commit only if both eye draws and full
    // restoration succeed; otherwise the transaction puts back original pixels.
    const bool cleanFrontend=ComposeCleanFrontend(dev,back,d,nativeMenu);
    g_frontMenuCleanFrame=cleanFrontend;

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
    unsigned pauseEyesDrawn = 0;
    const bool modalDialogEligible=modalmenu::CaptureExactDialog(
        Rd(g_modalSaveDialog)!=0,Rd(g_modalSlotsActive)!=0,Rd(g_vrModalDialogSurface)!=0);
    if (!cleanFrontend && nativeMenu && (!Rd(g_modalSaveDialog)||modalDialogEligible) && g_vrMenuTexture && g_vrMenuPanelValid &&
        (Rd(g_vrMenuClass) != 3 || dedicatedPause) && g_vrCaptureOwner.CanProvide(VrMenuStateToken())) {
        // RGB capture proves Objectives has nonzero RGB with zero stored alpha.
        // Preserve the native AlphaComposite RGB term instead of multiplying it
        // by alpha a second time and erasing Objectives.
        if (dedicatedPause) {
            if (PauseRttProbe::presentedToken != VrMenuStateToken()) {
                static unsigned stateLogs = 0;
                if (stateLogs++ < 6) {
                    DWORD index=0, transform=0, stage1=0, fog=0, stencil=0, blend=0;
                    dev->GetTextureStageState(0, D3DTSS_TEXCOORDINDEX, &index);
                    dev->GetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, &transform);
                    dev->GetTextureStageState(1, D3DTSS_COLOROP, &stage1);
                    dev->GetRenderState(D3DRS_FOGENABLE, &fog);
                    dev->GetRenderState(D3DRS_STENCILENABLE, &stencil);
                    dev->GetRenderState(D3DRS_BLENDOP, &blend);
                    Log("pause-rtt inherited uvIndex=%lu uvTransform=%lu stage1=%lu fog=%lu stencil=%lu blendOp=%lu",
                        index, transform, stage1, fog, stencil, blend);
                }
            }
            // GFx's last pass is not a fixed-function UI contract. In particular,
            // inherited TEXCOORDINDEX/transform or stage 1 can sample a different
            // region despite a successful DrawPrimitiveUP. StateBlock restores all.
            dev->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
            dev->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
            dev->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
            dev->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
            dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
            dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
            dev->SetRenderState(D3DRS_FOGENABLE, FALSE);
            dev->SetRenderState(D3DRS_STENCILENABLE, FALSE);
            dev->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
            dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
            dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
            dev->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
        }
        for(int eye=0;eye<2;++eye) {
            VrMenuVtx v[4]; bool visible=true;
            for(unsigned k=0;k<4;++k) {
                uipointer::ScreenPoint p;
                if(!VrUiProjectEye(eye,g_vrMenuPanel.Corner(k),p)){visible=false;break;}
                const auto& crop=g_vrMenuPanel.canvas;
                v[k]={(eye+p.u)*d.Width*.5f,p.v*d.Height,0,1,0xFFFFFFFF,
                      dedicatedPause ? ((k&1)?1.f:0.f) : ((k&1)?crop.cropU1:crop.cropU0),
                      dedicatedPause ? ((k&2)?1.f:0.f) : ((k&2)?crop.cropV1:crop.cropV0)};
            }
            if(!visible)continue;
            InterlockedExchange(&g_vrMenuProviderDrawing,1);
            if(g_origSetTexture)g_origSetTexture(dev,0,g_vrMenuTexture);
            else dev->SetTexture(0,g_vrMenuTexture);
            dev->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1);
            const HRESULT menuDraw = g_origDrawPrimUP(dev,D3DPT_TRIANGLESTRIP,2,v,sizeof(VrMenuVtx));
            if (dedicatedPause && PauseRttProbe::presentedToken != VrMenuStateToken()) {
                static unsigned geometryLogs = 0;
                if (geometryLogs++ < 12)
                    Log("pause-rtt quad eye=%d back=%ux%u xy=(%.1f,%.1f)-(%.1f,%.1f) tan=(%.4f,%.4f) hr=%08lX",
                        eye, d.Width, d.Height, v[0].x, v[0].y, v[3].x, v[3].y,
                        g_targetTanX, g_targetTanY, menuDraw);
            }
            if (dedicatedPause && SUCCEEDED(menuDraw)) ++pauseEyesDrawn;
            if(Rd(g_vrMenuClass)==1)InterlockedIncrement(&g_vrCounters.bootMenuProvided);
            if(Rd(g_vrMenuClass)==2)InterlockedIncrement(&g_vrCounters.mainMenuProvided);
            if(g_origSetTexture)g_origSetTexture(dev,0,nullptr);
            else dev->SetTexture(0,nullptr);
            InterlockedExchange(&g_vrMenuProviderDrawing,0);
        }
        if (dedicatedPause) dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    }

    // The synthetic row stays visible even when the controller ray is currently
    // outside the menu plane; only hover/click feedback depends on a valid hit.
    if (menuRow && !panel) DrawPauseVrSettingsRow(dev, d);

    // ---- LASER + HIT MARKER ----
    if (menu && g_vrMenuPanelValid) {
        LONG pointerU=500000,pointerV=500000;
        ReadVrUiPair(pointerU,pointerV);
        const uipointer::V endpoint=uipointer::PointAtCanvasUv(
            g_vrMenuPanel.plane,pointerU/1000000.0f,pointerV/1000000.0f);
        if(Rd(g_vrUiValid))DrawVrMenuLaser(dev,d);
        if(Rd(g_vrUiEverHit))DrawVrMenuReticle(dev,d,endpoint,Rd(g_vrUiDown)!=0);
    }

    // Menu legends are intentionally omitted.  PS icons are reserved for
    // contextual gameplay prompts (for example Enter -> Cross when picking up
    // a note) so the menu remains the game's native layout.

    if (old) g_origSetRenderTarget(dev, 0, old);
    dev->SetDepthStencilSurface(depth);
    const HRESULT restoreState = g_wristState->Apply();
    if (dedicatedPause && pauseEyesDrawn == 2 && SUCCEEDED(restoreState)) {
        if (PauseRttProbe::presentedToken != VrMenuStateToken())
        {
            Log("pause-rtt provider token=%llu bothEyes=1 fullUV=1 alphaCompositeRGB=1", VrMenuStateToken());
            static unsigned captures = 0;
            if (captures++ < 3) InterlockedExchange(&g_pauseCaptureRequested, 1);
        }
        PauseRttProbe::presentedToken = VrMenuStateToken();
    } else if (Rd(g_vrMenuClass) == 3) PauseRttProbe::presentedToken = 0;
    if (old) old->Release(); if (depth) depth->Release(); back->Release();
}
