#define NOMINMAX
#include "../tmd_short_gauntlet.h"
#include <cassert>
#include <cstdio>

// Standalone hidden-window D3D9/Ex fixture, not the game. Original SVTG
// indices, synthetic VB positions. Proves the measured DIP range/binding and
// pixel production; NOT native material, tracking or headset correctness.
static void Run(const char* asset,bool ex) {
    HWND window=CreateWindowA("STATIC","SVTG offline",WS_OVERLAPPED,0,0,128,64,
        nullptr,nullptr,GetModuleHandle(nullptr),nullptr);
    assert(window);
    IDirect3D9* api=nullptr;IDirect3D9Ex* apiEx=nullptr;
    if(ex){assert(SUCCEEDED(Direct3DCreate9Ex(D3D_SDK_VERSION,&apiEx)));api=apiEx;}
    else api=Direct3DCreate9(D3D_SDK_VERSION);
    assert(api);
    D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;
    pp.hDeviceWindow=window;pp.BackBufferWidth=128;pp.BackBufferHeight=64;
    pp.BackBufferFormat=D3DFMT_X8R8G8B8;
    IDirect3DDevice9* dev=nullptr;
    if(ex){IDirect3DDevice9Ex* d=nullptr;
        assert(SUCCEEDED(apiEx->CreateDeviceEx(0,D3DDEVTYPE_HAL,window,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING,&pp,nullptr,&d)));dev=d;}
    else assert(SUCCEEDED(api->CreateDevice(0,D3DDEVTYPE_HAL,window,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING,&pp,&dev)));
    tmdgauntlet::Renderer renderer;assert(renderer.Load(asset));
    assert(renderer.Covers(2626,4818));
    auto* replacement=renderer.Ensure(dev);assert(replacement);
    IDirect3DIndexBuffer9* original=nullptr;
    assert(SUCCEEDED(dev->CreateIndexBuffer(36369*2,D3DUSAGE_WRITEONLY,D3DFMT_INDEX16,
        D3DPOOL_DEFAULT,&original,nullptr)));
    assert(SUCCEEDED(dev->SetIndices(original)));
    struct V {float x,y,z;DWORD color;};
    IDirect3DVertexBuffer9* vb=nullptr;
    assert(SUCCEEDED(dev->CreateVertexBuffer(7444*sizeof(V),D3DUSAGE_WRITEONLY,
        D3DFVF_XYZ|D3DFVF_DIFFUSE,D3DPOOL_DEFAULT,&vb,nullptr)));
    V* v=nullptr;assert(SUCCEEDED(vb->Lock(0,0,reinterpret_cast<void**>(&v),0)));
    // Dense grid ensures many nondegenerate triangles for the original IB.
    for(unsigned i=0;i<7444;++i)v[i]={float(i%23)/12.f-.9f,float(i%17)/9.f-.9f,.5f,0xffffffff};
    assert(SUCCEEDED(vb->Unlock()));
    assert(SUCCEEDED(dev->SetStreamSource(0,vb,0,sizeof(V))));
    assert(SUCCEEDED(dev->SetFVF(D3DFVF_XYZ|D3DFVF_DIFFUSE)));
    D3DMATRIX identity{};identity._11=identity._22=identity._33=identity._44=1;
    dev->SetTransform(D3DTS_WORLD,&identity);dev->SetTransform(D3DTS_VIEW,&identity);
    dev->SetTransform(D3DTS_PROJECTION,&identity);
    dev->SetRenderState(D3DRS_LIGHTING,FALSE);dev->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
    dev->SetRenderState(D3DRS_ZENABLE,FALSE);
    dev->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);
    dev->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);
    assert(SUCCEEDED(dev->Clear(0,nullptr,D3DCLEAR_TARGET,0,1,0)));
    assert(SUCCEEDED(dev->BeginScene()));
    unsigned draws=0;HRESULT draw=S_OK,restore=E_FAIL;
    {
        nativegloves::IndexGuard<> guard(dev,replacement,36369*2);assert(guard.valid);
        for(unsigned eye=0;eye<2;++eye){
            D3DVIEWPORT9 vp{eye*64,0,64,64,0,1};assert(SUCCEEDED(dev->SetViewport(&vp)));
            const auto r=tmdgauntlet::DispatchBound(true,guard.valid,renderer.Triangles(),0,
                [&](unsigned first,unsigned count){
                    assert(count==7110);const HRESULT h=dev->DrawIndexedPrimitive(
                        D3DPT_TRIANGLELIST,0,2626,4818,first,count);
                    if(SUCCEEDED(h))++draws;return h;},
                [](unsigned,unsigned){assert(false);return E_FAIL;});
            if(FAILED(r.hr))draw=r.hr;
        }
        restore=guard.Restore();
    }
    assert(SUCCEEDED(dev->EndScene()));
    assert(tmdgauntlet::SuccessfulStereoBoundDraw(true,draws,draw,restore));
    IDirect3DIndexBuffer9* restored=nullptr;dev->GetIndices(&restored);
    assert(restored==original);restored->Release();
    IDirect3DSurface9 *rt=nullptr,*read=nullptr;
    assert(SUCCEEDED(dev->GetRenderTarget(0,&rt)));
    assert(SUCCEEDED(dev->CreateOffscreenPlainSurface(128,64,D3DFMT_X8R8G8B8,
        D3DPOOL_SYSTEMMEM,&read,nullptr)));
    assert(SUCCEEDED(dev->GetRenderTargetData(rt,read)));
    D3DLOCKED_RECT lock{};assert(SUCCEEDED(read->LockRect(&lock,nullptr,D3DLOCK_READONLY)));
    unsigned pixels[2]{};
    for(unsigned y=0;y<64;++y)for(unsigned x=0;x<128;++x)
        if(reinterpret_cast<DWORD*>(static_cast<char*>(lock.pBits)+y*lock.Pitch)[x]&0xffffff)++pixels[x/64];
    assert(pixels[0]>100 && pixels[0]==pixels[1]);
    read->UnlockRect();read->Release();rt->Release();
    renderer.ReleaseGPU();dev->SetIndices(nullptr);dev->SetStreamSource(0,nullptr,0,0);
    original->Release();vb->Release();dev->Release();api->Release();DestroyWindow(window);
    std::printf("PASS D3D9%s: min=2626 verts=4818, 2x7110 draws, IB restored, pixels=%u/%u\n",
        ex?"Ex":"",pixels[0],pixels[1]);
}
int main(int argc,char** argv){assert(argc==2);Run(argv[1],false);Run(argv[1],true);}
