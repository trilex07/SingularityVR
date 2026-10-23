#define NOMINMAX
#include <windows.h>
#include <d3d9.h>
#include "../settings_panel.h"
#include "../ui_remaster_runtime.h"
#include <cassert>
#include <cstdio>

// Off-game fixture. Uses the same sprite UV and per-eye rectangle as the M02
// adapter. A device-creation failure is an environment BLOCK, not pixel PASS.
static int Run(bool ex){
    HWND window=CreateWindowA("STATIC","M02 offline",WS_OVERLAPPED,0,0,128,64,
        nullptr,nullptr,GetModuleHandle(nullptr),nullptr);
    if(!window)return 2;
    IDirect3D9* api=nullptr;IDirect3D9Ex* apiEx=nullptr;
    if(ex){if(FAILED(Direct3DCreate9Ex(D3D_SDK_VERSION,&apiEx)))return 3;api=apiEx;}
    else api=Direct3DCreate9(D3D_SDK_VERSION);
    if(!api)return 3;
    D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;
    pp.hDeviceWindow=window;pp.BackBufferWidth=128;pp.BackBufferHeight=64;
    pp.BackBufferFormat=D3DFMT_X8R8G8B8;
    IDirect3DDevice9* dev=nullptr;HRESULT create=E_FAIL;
    if(ex){IDirect3DDevice9Ex* d=nullptr;create=apiEx->CreateDeviceEx(0,D3DDEVTYPE_HAL,window,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING,&pp,nullptr,&d);dev=d;}
    else create=api->CreateDevice(0,D3DDEVTYPE_HAL,window,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING,&pp,&dev);
    if(FAILED(create)||!dev){std::printf("BLOCKED D3D9%s CreateDevice HAL hr=%08lx\n",ex?"Ex":"",create);
        api->Release();DestroyWindow(window);return 4;}
    IDirect3DTexture9* tex=nullptr;
    assert(SUCCEEDED(dev->CreateTexture(2,2,1,D3DUSAGE_DYNAMIC,D3DFMT_A8R8G8B8,
        D3DPOOL_DEFAULT,&tex,nullptr)));
    D3DLOCKED_RECT lock{};assert(SUCCEEDED(tex->LockRect(0,&lock,nullptr,D3DLOCK_DISCARD)));
    for(int y=0;y<2;++y)for(int x=0;x<2;++x)
        ((DWORD*)((BYTE*)lock.pBits+y*lock.Pitch))[x]=0xff38a472;
    tex->UnlockRect(0);
    IDirect3DStateBlock9* state=nullptr;
    assert(SUCCEEDED(dev->CreateStateBlock(D3DSBT_ALL,&state)));
    assert(SUCCEEDED(dev->Clear(0,nullptr,D3DCLEAR_TARGET,0,1,0)));
    DWORD before=0;assert(SUCCEEDED(dev->GetRenderState(D3DRS_ALPHABLENDENABLE,&before)));
    assert(SUCCEEDED(state->Capture()));
    dev->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);
    dev->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
    dev->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
    dev->SetRenderState(D3DRS_ZENABLE,FALSE);
    dev->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);
    dev->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);
    dev->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);
    dev->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);
    dev->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);
    dev->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1);dev->SetTexture(0,tex);
    assert(SUCCEEDED(dev->BeginScene()));
    unsigned draws=0;const auto uv=uiremaster::settingsPlate;
    for(int eye=0;eye<2;++eye){
        // Reduced-size equivalent of the production shared plate geometry.
        const auto q=uiremaster::Plate(eye,2,64,48,32,16,1);
        assert(uiremaster::Valid(q,eye,64,128,64));
        uiremaster::Vertex v[4]{};uiremaster::BuildQuad(q,uv,v);
        if(SUCCEEDED(dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,v,sizeof(v[0]))))++draws;
        // P1 selected row and localized tab use the same panel/eye coordinates.
        const auto focus=uiremaster::Focus(q,16,13,1,0);
        assert(uiremaster::Valid(focus,eye,64,128,64));
        uiremaster::BuildQuad(focus,uiremaster::focus,v);
        assert(SUCCEEDED(dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,v,sizeof(v[0]))));
        const uiremaster::Rect tab{q.right-22,q.top+2,q.right-2,q.top+8};
        assert(uiremaster::Valid(tab,eye,64,128,64));
        uiremaster::BuildQuad(tab,uiremaster::Tab(3),v);
        assert(SUCCEEDED(dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,v,sizeof(v[0]))));
    }
    assert(SUCCEEDED(dev->EndScene()));assert(draws==2);
    assert(SUCCEEDED(state->Apply()));
    DWORD after=0;assert(SUCCEEDED(dev->GetRenderState(D3DRS_ALPHABLENDENABLE,&after))&&after==before);
    IDirect3DBaseTexture9* rebound=nullptr;assert(SUCCEEDED(dev->GetTexture(0,&rebound)));
    assert(rebound!=tex);if(rebound)rebound->Release();
    IDirect3DSurface9 *rt=nullptr,*read=nullptr;
    assert(SUCCEEDED(dev->GetRenderTarget(0,&rt)));
    assert(SUCCEEDED(dev->CreateOffscreenPlainSurface(128,64,D3DFMT_X8R8G8B8,
        D3DPOOL_SYSTEMMEM,&read,nullptr)));
    assert(SUCCEEDED(dev->GetRenderTargetData(rt,read)));
    assert(SUCCEEDED(read->LockRect(&lock,nullptr,D3DLOCK_READONLY)));
    unsigned pixels[2]{};
    for(unsigned y=0;y<64;++y)for(unsigned x=0;x<128;++x)
        if(((DWORD*)((BYTE*)lock.pBits+y*lock.Pitch))[x]&0xffffff)++pixels[x/64];
    assert(pixels[0]>100&&pixels[0]==pixels[1]);
    read->UnlockRect();read->Release();rt->Release();state->Release();tex->Release();dev->Release();
    api->Release();DestroyWindow(window);
    std::printf("M02 D3D9%s PASS: 2 eyes, %u/%u pixels, texture/blend state restored\n",
        ex?"Ex":"",pixels[0],pixels[1]);return 0;
}
int main(){const int a=Run(false);if(a)return a;return Run(true);}
