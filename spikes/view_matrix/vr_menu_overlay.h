#pragma once
// Included after the D3D state/overlay helpers. No per-frame allocation or scan.
static IDirect3DTexture9* g_vrMenuTexture=nullptr;
static uintptr_t g_vrMenuSurface=0;
static unsigned g_vrMenuFrame=0,g_vrMenuTextureFrame=0;
static uipointer::Hit g_vrUiHit;
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
    if(fh->bfType!=0x4D42||ih->biSize<sizeof(BITMAPINFOHEADER)||ih->biWidth!=320||ih->biHeight!=64||ih->biBitCount!=32||ih->biCompression!=BI_RGB||fh->bfOffBits>=size){free(bytes);return false;}
    IDirect3DTexture9* tex=nullptr;
    HRESULT hr=g_origCreateTexture ? g_origCreateTexture(dev,320,64,1,D3DUSAGE_DYNAMIC,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&tex,nullptr) : dev->CreateTexture(320,64,1,D3DUSAGE_DYNAMIC,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&tex,nullptr);
    if(FAILED(hr)||!tex){free(bytes);Log("VR prompt atlas texture creation failed: 0x%08lX",hr);return false;}
    D3DLOCKED_RECT lock{};
    if(FAILED(tex->LockRect(0,&lock,nullptr,0))){tex->Release();free(bytes);return false;}
    const unsigned char* src=bytes+fh->bfOffBits;
    for(int y=0;y<64;++y) memcpy((unsigned char*)lock.pBits+y*lock.Pitch,src+(63-y)*320*4,320*4);
    tex->UnlockRect(0); free(bytes); g_vrPromptAtlas=tex;
    Log("VR prompt atlas loaded: %s",path); return true;
}
static void DrawVrPromptIcon(IDirect3DDevice9* dev,float x,float y,float size,float u0,float u1) {
    VrPromptVtx v[4]={{x,y,0,1,0xFFFFFFFF,u0,0},{x+size,y,0,1,0xFFFFFFFF,u1,0},{x,y+size,0,1,0xFFFFFFFF,u0,1},{x+size,y+size,0,1,0xFFFFFFFF,u1,1}};
    g_origDrawPrimUP(dev,D3DPT_TRIANGLESTRIP,2,v,sizeof(VrPromptVtx));
}
static void DrawVrPromptLegend(IDirect3DDevice9* dev,const D3DSURFACE_DESC& d,bool gameplay) {
    if(!LoadVrPromptAtlas(dev)) return;
    dev->SetTexture(0,g_vrPromptAtlas); dev->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1);
    dev->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE); dev->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE); dev->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_DIFFUSE);
    dev->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_MODULATE); dev->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE); dev->SetTextureStageState(0,D3DTSS_ALPHAARG2,D3DTA_DIFFUSE);
    dev->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_POINT); dev->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_POINT);
    const float eyeW=d.Width*.5f, size=std::max(22.f,d.Height*.035f), y=d.Height*.83f;
    for(int eye=0;eye<2;++eye){
        const float base=eye*eyeW+eyeW*.12f;
        DrawVrPromptIcon(dev,base,y,size,0.f,.2f);
        if(gameplay) {
            DrawVrPromptIcon(dev,base+eyeW*.22f,y,size,.6f,.8f);
            DrawVrPromptIcon(dev,base+eyeW*.44f,y,size,.4f,.6f);
            DrawVrPromptIcon(dev,base+eyeW*.66f,y,size,.2f,.4f);
        } else {
            DrawVrPromptIcon(dev,base+eyeW*.25f,y,size,.2f,.4f);
            DrawVrPromptIcon(dev,base+eyeW*.50f,y,size,.8f,1.f);
        }
    }
    dev->SetTexture(0,nullptr); dev->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE);
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
void UpdateVrUiPointer(float,bool available) {
    const bool menu=Rd(g_vrUiEnabled)&&Rd(g_vrUiNativeReady)&&Rd(g_vrUiMenu);
    if(menu&&Rd(g_vrUiDesktopProbe))return; // Explicit debugger-controlled test coordinates; never staged for headset use.
    if(!available || !menu || !g_handPoseValid[1] || !g_handPositionValid[1] || !g_haveEyes || g_targetTanX<=0 || g_targetTanY<=0 || Rd(g_panelOpen)) {
        InterlockedExchange(&g_vrUiValid,0);InterlockedExchange(&g_vrUiDown,0);
        if(!available)for(auto& gate:g_vrUiTrigger)gate.Update(menu?uipointer::Context::Menu:uipointer::Context::Gameplay,false,false,false);
        return;
    }
    const auto& q=g_eyePose[0].orientation;
    uipointer::V head{(g_eyePose[0].position.x+g_eyePose[1].position.x)*.5f,(g_eyePose[0].position.y+g_eyePose[1].position.y)*.5f,(g_eyePose[0].position.z+g_eyePose[1].position.z)*.5f};
    uipointer::Plane plane;uipointer::Hit hit;
    const auto& pos=g_handLastPos[1];
    const float inset=Rd(g_menuInsetPct)/100.f;
    const float distance=Rd(g_vrMenuDistanceMm)/1000.0f;
    bool valid=uipointer::HeadPlane(head,VrUiRotate(q,{0,0,-1}),VrUiRotate(q,{1,0,0}),VrUiRotate(q,{0,1,0}),distance,g_targetTanX,g_targetTanY,inset,plane) &&
        uipointer::Intersect({pos.x,pos.y,pos.z},VrUiRotate(g_handAimOrientation[1],{0,0,-1}),plane,2,2,hit);
    if(valid){
        g_vrUiHit=hit;InterlockedExchange(&g_vrUiU,(LONG)lroundf(hit.u*1000000));InterlockedExchange(&g_vrUiV,(LONG)lroundf(hit.v*1000000));
        for(int eye=0;eye<2;++eye){uipointer::ScreenPoint c;if(VrUiProjectEye(eye,plane.centre,c))InterlockedExchange(&g_vrUiEyeCentreU[eye],(LONG)lroundf(c.u*1000000));}
    }
    else InterlockedExchange(&g_vrUiDown,0);
    InterlockedExchange(&g_vrUiValid,valid?1:0);
}
void CaptureVrMenuTarget(IDirect3DDevice9* dev) {
    if(!Rd(g_vrUiEnabled)||!Rd(g_vrUiMenu)||!Rd(g_vrUiNativeReady)||g_boundShaderLayout.shadow)return;
    static unsigned frame=~0u,target=~0u;
    if(frame==g_vrMenuFrame&&target==g_vrUiTargetEpoch)return;
    // GFx menu surfaces are not guaranteed to carry the identity c5 orthographic constants.
    // Requiring that matrix silently missed the live menu and left only its lower-right diegetic
    // monitor visible. Read c5 for diagnostics, but identify the target by bounded colour format
    // and dimensions below so it can be copied to the centered head-relative overlay.
    float c[16];if(FAILED(dev->GetVertexShaderConstantF(5,c,4)))return;
    frame=g_vrMenuFrame;target=g_vrUiTargetEpoch;
    IDirect3DSurface9* surface=nullptr;if(FAILED(dev->GetRenderTarget(0,&surface))||!surface)return;
    D3DSURFACE_DESC desc{};surface->GetDesc(&desc);
    if(desc.Width<256||desc.Height<256||desc.Width>2048||desc.Height>2048||
       (desc.Format!=D3DFMT_A8R8G8B8&&desc.Format!=D3DFMT_X8R8G8B8)){surface->Release();return;}
    if(g_vrMenuSurface!=(uintptr_t)surface){
        IDirect3DTexture9* texture=nullptr;
        if(SUCCEEDED(surface->GetContainer(__uuidof(IDirect3DTexture9),(void**)&texture))&&texture){
            if(g_vrMenuTexture)g_vrMenuTexture->Release();g_vrMenuTexture=texture;g_vrMenuSurface=(uintptr_t)surface;
            Log("VR menu head-relative source: GFx target %ux%u; native viewport coordinates",desc.Width,desc.Height);
        }
    }
    if(g_vrMenuTexture && g_vrMenuSurface==(uintptr_t)surface)g_vrMenuTextureFrame=g_vrMenuFrame;
    surface->Release();
}
void ReleaseVrMenuResources(){if(g_vrMenuTexture){g_vrMenuTexture->Release();g_vrMenuTexture=nullptr;}if(g_vrPromptAtlas){g_vrPromptAtlas->Release();g_vrPromptAtlas=nullptr;}g_vrPromptAtlasAttempted=false;g_vrMenuSurface=0;}
// Render the captured GFx menu texture as a single head-relative quad at VrMenuDistanceMm.
// Also draws the laser pointer and prompt legend.
void DrawVrMenuPointer(IDirect3DDevice9* dev) {
    ++g_vrMenuFrame;
    const bool menu = Rd(g_vrUiEnabled) && VrModeOn() && Rd(g_dupDraws) && Rd(g_vrUiNativeReady) && Rd(g_vrUiMenu);
    const bool gameplayHints = Rd(g_vrUiEnabled) && VrModeOn() && Rd(g_dupDraws) && Rd(g_vrUiNativeReady) && !Rd(g_vrUiMenu) && Rd(g_lastMenuAge) < 180;
    if ((!menu && !gameplayHints) || Rd(g_panelOpen) || !g_origDrawPrimUP || !g_origSetVS || !g_origSetRenderTarget) return;

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
    if (menu && g_vrMenuTexture && g_vrMenuTextureFrame == g_vrMenuFrame) {
        int slot = -1;
        for (int i = 0; i < g_camMatUsed; ++i) if (g_camMats[i].valid) { slot = i; break; }
        if (slot >= 0) {
            const float distance = Rd(g_vrMenuDistanceMm) / 1000.0f;
            const float inset = Rd(g_menuInsetPct) / 100.0f;
            const auto& q = g_eyePose[0].orientation;
            uipointer::V head{ (g_eyePose[0].position.x + g_eyePose[1].position.x) * .5f,
                                (g_eyePose[0].position.y + g_eyePose[1].position.y) * .5f,
                                (g_eyePose[0].position.z + g_eyePose[1].position.z) * .5f };
            uipointer::V fwd = VrUiRotate(q, {0, 0, -1});
            uipointer::V right = VrUiRotate(q, {1, 0, 0});
            uipointer::V up = VrUiRotate(q, {0, 1, 0});
            uipointer::Plane plane;
            if (uipointer::HeadPlane(head, fwd, right, up, distance, g_targetTanX, g_targetTanY, inset, plane)) {
                for (int eye = 0; eye < 2; ++eye) {
                    float m[16]; memcpy(m, g_camMats[slot].m, sizeof(m)); const int conv = g_camMats[slot].conv;
                    if (eye) ApplyOffset(reinterpret_cast<Reg4*>(m), conv, g_eyeDeltaUU);
                    ApplyEyeRemap(reinterpret_cast<Reg4*>(m), conv, eye ? .5f : -.5f);

                    // Build menu quad vertices in tracking space
                    VrMenuVtx v[4];
                    const uipointer::V corners[4] = {
                        plane.centre + right * (-plane.halfWidth * inset) + up * (plane.halfHeight * inset),   // top-left
                        plane.centre + right * (plane.halfWidth * inset) + up * (plane.halfHeight * inset),    // top-right
                        plane.centre + right * (-plane.halfWidth * inset) + up * (-plane.halfHeight * inset),  // bottom-left
                        plane.centre + right * (plane.halfWidth * inset) + up * (-plane.halfHeight * inset),   // bottom-right
                    };
                    bool visible = true;
                    for (int k = 0; k < 4; ++k) {
                        float clip[4]; WorldToClip(m, conv, &corners[k].x, clip);
                        if (clip[3] <= .001f) { visible = false; break; }
                        v[k] = { (clip[0] / clip[3] * .5f + .5f) * vp.Width + vp.X,
                                 (.5f - clip[1] / clip[3] * .5f) * vp.Height + vp.Y,
                                 0, 1, 0xFFFFFFFF,
                                 (k & 1) ? 1.0f : 0.0f,  // u
                                 (k & 2) ? 1.0f : 0.0f }; // v
                    }
                    if (visible) {
                        dev->SetTexture(0, g_vrMenuTexture);
                        dev->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
                        g_origDrawPrimUP(dev, D3DPT_TRIANGLESTRIP, 2, v, sizeof(VrMenuVtx));
                        dev->SetTexture(0, nullptr);
                    }
                }
            }
        }
    }

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

    // ---- PROMPT LEGEND ----
    DrawVrPromptLegend(dev, d, gameplayHints);
    D3DRECT text[1024]; int n = 0; const int px = std::max(1, (int)d.Height / 1000);
    const char* labels = gameplayHints ? "FIRE   USE   JUMP   CROUCH" : "SELECT   BACK   PAUSE";
    for (int eye = 0; eye < 2; ++eye) { int x = (int)(eye * d.Width * .5f + d.Width * .12f); int y = (int)(d.Height * .90f); n = TextRects(text, n, 1024, x, y, px, labels); }
    ClearRects(dev, text, n, 0xFFB8FFD0);

    if (old) g_origSetRenderTarget(dev, 0, old);
    dev->SetDepthStencilSurface(depth);
    g_wristState->Apply();
    if (old) old->Release(); if (depth) depth->Release(); back->Release();
}
