#define NOMINMAX
#include <windows.h>
#include <d3d9.h>
#include "../wheel_draw_state.h"
#include "../ui_remaster_runtime.h"
#include <array>
#include <cassert>
#include <cstdio>

// Actual atlas -> premultiplied scratch -> two-eye backbuffer pixels, using
// production state setup. No game/Steam. Poison one inherited material state
// at a time; reproduce the incomplete old setup as a negative control.
static int Run(bool ex) {
    HWND w=CreateWindowA("STATIC","wheel offline",WS_OVERLAPPED,0,0,128,64,
        nullptr,nullptr,GetModuleHandle(nullptr),nullptr);assert(w);
    IDirect3D9* api=nullptr;IDirect3D9Ex* ae=nullptr;
    if(ex){assert(SUCCEEDED(Direct3DCreate9Ex(D3D_SDK_VERSION,&ae)));api=ae;}
    else api=Direct3DCreate9(D3D_SDK_VERSION);
    assert(api);D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;
    pp.hDeviceWindow=w;pp.BackBufferWidth=128;pp.BackBufferHeight=64;pp.BackBufferFormat=D3DFMT_X8R8G8B8;
    IDirect3DDevice9* d=nullptr;HRESULT hr=E_FAIL;
    if(ex){IDirect3DDevice9Ex* de=nullptr;hr=ae->CreateDeviceEx(0,D3DDEVTYPE_HAL,w,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING,&pp,nullptr,&de);d=de;}
    else hr=api->CreateDevice(0,D3DDEVTYPE_HAL,w,D3DCREATE_SOFTWARE_VERTEXPROCESSING,&pp,&d);
    if(FAILED(hr)){printf("BLOCKED wheel D3D9%s HAL=%08lX\n",ex?"Ex":"",hr);api->Release();DestroyWindow(w);return 4;}
    IDirect3DTexture9 *atlas=nullptr,*scratch=nullptr;IDirect3DSurface9 *stage=nullptr,*back=nullptr,*read=nullptr;
    assert(SUCCEEDED(d->CreateTexture(2,2,1,D3DUSAGE_DYNAMIC,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&atlas,nullptr)));
    D3DLOCKED_RECT lr{};assert(SUCCEEDED(atlas->LockRect(0,&lr,nullptr,D3DLOCK_DISCARD)));
    for(int y=0;y<2;++y)for(int x=0;x<2;++x)((DWORD*)((char*)lr.pBits+y*lr.Pitch))[x]=0x804080c0;
    atlas->UnlockRect(0);
    assert(SUCCEEDED(d->CreateTexture(32,32,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&scratch,nullptr)));
    assert(SUCCEEDED(scratch->GetSurfaceLevel(0,&stage)));assert(SUCCEEDED(d->GetRenderTarget(0,&back)));
    assert(SUCCEEDED(d->CreateOffscreenPlainSurface(128,64,D3DFMT_X8R8G8B8,D3DPOOL_SYSTEMMEM,&read,nullptr)));
    IDirect3DStateBlock9* saved=nullptr;assert(SUCCEEDED(d->CreateStateBlock(D3DSBT_ALL,&saved)));
    const D3DVIEWPORT9 outer{0,0,128,64,0,1},inner{0,0,32,32,0,1};
    using Pixels=std::array<DWORD,128*64>;
    const auto readback=[&]{Pixels p{};assert(SUCCEEDED(d->GetRenderTargetData(back,read)));
        assert(SUCCEEDED(read->LockRect(&lr,nullptr,D3DLOCK_READONLY)));
        for(int y=0;y<64;++y)for(int x=0;x<128;++x)p[y*128+x]=((DWORD*)((char*)lr.pBits+y*lr.Pitch))[x]&0xffffff;
        read->UnlockRect();return p;};
    const auto inherited=[&](unsigned poison){
        assert(wheelstate::Prepare(d,wheelstate::Pass::Legacy));
        if(poison==1){d->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_REVSUBTRACT);
            d->SetRenderState(D3DRS_BLENDOPALPHA,D3DBLENDOP_REVSUBTRACT);}
        if(poison==2){d->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_MODULATE);
            d->SetTextureStageState(1,D3DTSS_COLORARG1,D3DTA_CURRENT);
            d->SetTextureStageState(1,D3DTSS_COLORARG2,D3DTA_TFACTOR);
            d->SetRenderState(D3DRS_TEXTUREFACTOR,0xff000000);}
        if(poison==3){d->SetRenderState(D3DRS_FOGENABLE,TRUE);d->SetRenderState(D3DRS_FOGCOLOR,0);
            d->SetRenderState(D3DRS_FOGTABLEMODE,D3DFOG_LINEAR);}
        if(poison==4)d->SetSamplerState(0,D3DSAMP_SRGBTEXTURE,TRUE);
        if(poison==5){d->SetRenderState(D3DRS_ALPHATESTENABLE,TRUE);d->SetRenderState(D3DRS_ALPHAFUNC,D3DCMP_NEVER);}
        if(poison==6)d->SetRenderState(D3DRS_COLORWRITEENABLE,0);
    };
    const auto render=[&](unsigned poison,bool fixed,bool abort)->Pixels {
        inherited(poison);d->SetTexture(0,atlas);d->SetViewport(&outer);
        assert(SUCCEEDED(saved->Capture()));
        d->SetRenderTarget(0,back);d->Clear(0,nullptr,D3DCLEAR_TARGET,0xff203040,1,0);
        const auto unchanged=readback();
        assert(SUCCEEDED(d->SetRenderTarget(0,stage)));assert(SUCCEEDED(d->SetViewport(&inner)));
        d->Clear(0,nullptr,D3DCLEAR_TARGET,0,1,0);
        if(fixed)assert(wheelstate::Prepare(d,wheelstate::Pass::Atlas));
        else {
            // Old production wheel configured these, but inherited fog, stage1,
            // sRGB sampling, blend operations, alpha-test and write mask.
            d->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);
            d->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);d->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
            d->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE,TRUE);
            d->SetRenderState(D3DRS_SRCBLENDALPHA,D3DBLEND_ONE);d->SetRenderState(D3DRS_DESTBLENDALPHA,D3DBLEND_INVSRCALPHA);
            d->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);
        }
        d->SetTexture(0,atlas);d->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1);
        uiremaster::Vertex q[4]{};uiremaster::BuildQuad({4,4,28,28},{0,0,1,1},q);
        assert(SUCCEEDED(d->BeginScene()));assert(SUCCEEDED(d->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,q,sizeof(q[0]))));
        const D3DRECT text{14,14,18,18};d->Clear(1,&text,D3DCLEAR_TARGET,0xffffffff,1,0);
        assert(SUCCEEDED(d->EndScene()));
        assert(SUCCEEDED(d->SetRenderTarget(0,back)));assert(SUCCEEDED(d->SetViewport(&outer)));
        if(!abort){
            if(fixed)assert(wheelstate::Prepare(d,wheelstate::Pass::Composite));
            else {d->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE,FALSE);d->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_ONE);}
            const int cx[2]{32,96},cy[2]{32,32};uiremaster::Vertex pair[12]{};
            assert(uiremaster::BuildWheelPair(cx,cy,8,128,64,32,pair));
            d->SetTexture(0,scratch);d->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1);
            assert(SUCCEEDED(d->BeginScene()));assert(SUCCEEDED(d->DrawPrimitiveUP(D3DPT_TRIANGLELIST,4,pair,sizeof(pair[0]))));
            assert(SUCCEEDED(d->EndScene()));
        }
        auto pixels=readback();if(abort)assert(pixels==unchanged);
        assert(SUCCEEDED(saved->Apply()));d->SetRenderTarget(0,back);d->SetViewport(&outer);
        DWORD v=0;d->GetRenderState(D3DRS_BLENDOP,&v);assert(v==(poison==1?D3DBLENDOP_REVSUBTRACT:D3DBLENDOP_ADD));
        d->GetTextureStageState(1,D3DTSS_COLOROP,&v);assert(v==(poison==2?D3DTOP_MODULATE:D3DTOP_DISABLE));
        d->GetRenderState(D3DRS_FOGENABLE,&v);assert(v==(poison==3));
        d->GetSamplerState(0,D3DSAMP_SRGBTEXTURE,&v);assert(v==(poison==4));
        d->GetRenderState(D3DRS_ALPHATESTENABLE,&v);assert(v==(poison==5));
        d->GetRenderState(D3DRS_COLORWRITEENABLE,&v);assert(v==(poison==6?0u:15u));
        IDirect3DBaseTexture9* rebound=nullptr;d->GetTexture(0,&rebound);assert(rebound==atlas);rebound->Release();
        return pixels;
    };
    const auto baseline=render(0,true,false);
    assert(baseline[32*128+32]==0xffffff);assert(baseline[0]==0x203040);
    for(int y=0;y<64;++y)for(int x=0;x<64;++x)assert(baseline[y*128+x]==baseline[y*128+x+64]);
    for(unsigned p=1;p<=6;++p){const auto broken=render(p,false,false),correct=render(p,true,false);
        if(p!=3)assert(broken!=baseline); // Fog support for XYZRHW varies by driver.
        assert(correct==baseline);render(p,true,true);
        printf("wheel D3D9%s poison=%u oldDiff=%d corrected=bit-identical stereo; state/abort restored\n",ex?"Ex":"",p,int(broken!=baseline));
    }
    d->SetTexture(0,nullptr);saved->Release();read->Release();stage->Release();scratch->Release();atlas->Release();back->Release();
    assert(SUCCEEDED(d->Reset(&pp)));assert(wheelstate::Prepare(d,wheelstate::Pass::Atlas));
    d->Release();api->Release();DestroyWindow(w);return 0;
}
int main(){int r=Run(false);return r?r:Run(true);}
