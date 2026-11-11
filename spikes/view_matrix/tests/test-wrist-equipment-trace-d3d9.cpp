#define NOMINMAX
#include <windows.h>
#include <d3d9.h>
#include <cassert>
#include <cstdio>
#include <cstring>
static unsigned logs=0;
static void Log(const char*,...){++logs;}
#include "../wrist_equipment_trace.h"
static void Run(bool ex){
    HWND w=CreateWindowA("STATIC","Offline wrist trace",WS_OVERLAPPED,0,0,256,128,0,0,GetModuleHandle(nullptr),0);assert(w);
    IDirect3D9* api=nullptr;IDirect3D9Ex* ax=nullptr;
    if(ex){assert(SUCCEEDED(Direct3DCreate9Ex(D3D_SDK_VERSION,&ax)));api=ax;}
    else api=Direct3DCreate9(D3D_SDK_VERSION);assert(api);
    D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;
    pp.BackBufferWidth=256;pp.BackBufferHeight=128;pp.BackBufferFormat=D3DFMT_X8R8G8B8;pp.hDeviceWindow=w;
    pp.EnableAutoDepthStencil=TRUE;pp.AutoDepthStencilFormat=D3DFMT_D24S8;
    IDirect3DDevice9* dev=nullptr;
    if(ex){IDirect3DDevice9Ex* dx=nullptr;assert(SUCCEEDED(ax->CreateDeviceEx(0,D3DDEVTYPE_HAL,w,D3DCREATE_HARDWARE_VERTEXPROCESSING,&pp,0,&dx)));dev=dx;}
    else assert(SUCCEEDED(api->CreateDevice(0,D3DDEVTYPE_HAL,w,D3DCREATE_HARDWARE_VERTEXPROCESSING,&pp,&dev)));
    wristtrace::done=false;wristtrace::stable=0;
    for(unsigned i=0;i<12000;++i){assert(!wristtrace::Eye(nullptr,1,0,0,0,0,0));
        assert(!wristtrace::Clear(nullptr,1,0,D3DCLEAR_ZBUFFER,1));}
    assert(!wristtrace::count);
    for(unsigned i=0;i<17;++i)wristtrace::Boundary(dev,true,true,i);
    assert(!wristtrace::active);wristtrace::Boundary(dev,true,true,18);assert(wristtrace::active);
    assert(SUCCEEDED(dev->BeginScene()));
    D3DVIEWPORT9 vp{0,0,256,128,0,1};RECT sc{0,0,128,128};float m[16]{};m[0]=m[5]=m[10]=m[15]=1;
    assert(SUCCEEDED(dev->SetViewport(&vp)));dev->SetScissorRect(&sc);dev->SetVertexShaderConstantF(0,m,4);
    IDirect3DSurface9 *rt=nullptr,*ds=nullptr;dev->GetRenderTarget(0,&rt);dev->GetDepthStencilSurface(&ds);
    auto* l=wristtrace::Eye(dev,10,0,0,0,0,0);assert(l&&l->depth==ds&&l->rt==rt);
    assert(l->dd.Format==D3DFMT_D24S8&&l->dd.Width==256&&SUCCEEDED(l->matrixHr));
    assert(!memcmp(l->matrix,m,sizeof m));
    assert(!wristtrace::Eye(dev,11,0,0,0,0,0));
    sc={128,0,256,128};dev->SetScissorRect(&sc);
    auto* r=wristtrace::Eye(dev,10,1,0,0,0,0);assert(r&&r->sc.left==128);
    auto* clear=wristtrace::Clear(dev,40,1,D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL,1);assert(clear&&clear->depth==ds);
    // Capture is read-only even inside BeginScene. Clear remains native-owned.
    clear->hr=dev->Clear(0,0,D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL,0,1,0);assert(SUCCEEDED(clear->hr));
    assert(wristtrace::Eye(dev,41,0,1,3,0,0));assert(wristtrace::Eye(dev,41,1,1,3,0,0));
    wristtrace::Wrist(dev,45,1000,999);
    const unsigned n=wristtrace::count;wristtrace::Wrist(dev,46,1001,1000);assert(wristtrace::count==n);
    IDirect3DSurface9 *rt2=nullptr,*ds2=nullptr;dev->GetRenderTarget(0,&rt2);dev->GetDepthStencilSurface(&ds2);
    D3DVIEWPORT9 after{};RECT afterSc{};float afterM[16]{};
    dev->GetViewport(&after);dev->GetScissorRect(&afterSc);dev->GetVertexShaderConstantF(0,afterM,4);
    assert(rt==rt2&&ds==ds2&&!memcmp(&vp,&after,sizeof vp)&&!memcmp(&sc,&afterSc,sizeof sc)&&!memcmp(m,afterM,sizeof m));
    rt->Release();ds->Release();rt2->Release();ds2->Release();assert(SUCCEEDED(dev->EndScene()));
    wristtrace::Boundary(dev,true,true,19);assert(!wristtrace::active&&wristtrace::done&&!wristtrace::count);
    for(unsigned i=0;i<100;++i)wristtrace::Boundary(dev,true,true,i+20);assert(!wristtrace::active);
    // Overflow and Reset release every retained COM reference before native Reset.
    wristtrace::active=true;
    for(unsigned i=0;i<80;++i)wristtrace::Capture(dev,"fixture",i,-1,0,0);
    assert(wristtrace::count==64&&wristtrace::overflow==16);wristtrace::Reset();
    assert(SUCCEEDED(dev->Reset(&pp)));
    dev->SetDepthStencilSurface(nullptr);wristtrace::active=true;
    auto* missing=wristtrace::Capture(dev,"missing-depth",1,-1,0,0);assert(missing&&!missing->depth&&FAILED(missing->depthHr));
    wristtrace::Reset();dev->Release();api->Release();DestroyWindow(w);
    printf("PASS %s actual read-only depth/eye/clear trace, bounded OFF, reference release, missing depth, Reset\n",ex?"D3D9Ex":"D3D9");
}
int main(){Run(false);Run(true);}
