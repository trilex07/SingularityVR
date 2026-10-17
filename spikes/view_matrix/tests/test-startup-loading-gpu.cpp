#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d9.h>
#include "../startup_loading_draw.h"
#include <cassert>
#include <cstdio>
#include <string>
#pragma comment(lib,"user32.lib")
template<class T>void Drop(T*& p){if(p)p->Release();p=nullptr;}
int main(){
    HWND w=CreateWindowExW(0,L"STATIC",L"OWN startup loading fixture",WS_OVERLAPPED,0,0,64,64,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    assert(w);char path[MAX_PATH]{};assert(GetSystemDirectoryA(path,MAX_PATH));
    HMODULE dll=LoadLibraryA((std::string(path)+"/d3d9.dll").c_str());assert(dll);
    using Factory=HRESULT(WINAPI*)(UINT,IDirect3D9Ex**);
    auto factory=reinterpret_cast<Factory>(GetProcAddress(dll,"Direct3DCreate9Ex"));assert(factory);
    IDirect3D9Ex* api=nullptr;assert(SUCCEEDED(factory(D3D_SDK_VERSION,&api)));
    D3DPRESENT_PARAMETERS pp{};pp.BackBufferWidth=512;pp.BackBufferHeight=256;pp.BackBufferFormat=D3DFMT_A8R8G8B8;
    pp.BackBufferCount=1;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.Windowed=TRUE;pp.hDeviceWindow=w;
    IDirect3DDevice9Ex* dev=nullptr;
    assert(SUCCEEDED(api->CreateDeviceEx(0,D3DDEVTYPE_HAL,w,D3DCREATE_HARDWARE_VERTEXPROCESSING,&pp,nullptr,&dev)));
    auto text=[](D3DRECT* r,int n,int cap,int x,int y,int px,const char*){assert(n<cap);r[n++]={x,y,x+13*6*px,y+7*px};return n;};
    for(unsigned round=0;round<2;++round){
        if(round)assert(SUCCEEDED(dev->ResetEx(&pp,nullptr)));
        IDirect3DSurface9 *rt=nullptr,*ds=nullptr,*back=nullptr,*read=nullptr;
        assert(SUCCEEDED(dev->CreateRenderTarget(64,64,D3DFMT_A8R8G8B8,D3DMULTISAMPLE_NONE,0,FALSE,&rt,nullptr)));
        assert(SUCCEEDED(dev->CreateDepthStencilSurface(64,64,D3DFMT_D24S8,D3DMULTISAMPLE_NONE,0,TRUE,&ds,nullptr)));
        assert(SUCCEEDED(dev->SetDepthStencilSurface(nullptr))&&SUCCEEDED(dev->SetRenderTarget(0,rt))&&SUCCEEDED(dev->SetDepthStencilSurface(ds)));
        const D3DVIEWPORT9 vp{3,5,32,32,0.1f,.9f};const RECT sc{4,6,8,9};
        assert(SUCCEEDED(dev->SetViewport(&vp))&&SUCCEEDED(dev->SetScissorRect(&sc))&&SUCCEEDED(dev->SetRenderState(D3DRS_SCISSORTESTENABLE,TRUE)));
        for(unsigned fail=0;fail<3;++fail){
            unsigned views=0;bool restored=false;
            const bool drawn=startup::DrawLoading(dev,
                [](IDirect3DDevice9* d,DWORD i,IDirect3DSurface9* s){return d->SetRenderTarget(i,s);},
                [&](IDirect3DDevice9* d,const D3DVIEWPORT9* v){return fail==1&&++views==1?E_FAIL:d->SetViewport(v);},
                [&](IDirect3DDevice9* d,DWORD n,const D3DRECT* r,DWORD f,D3DCOLOR c,float z,DWORD s){return fail==2?E_FAIL:d->Clear(n,r,f,c,z,s);},text,restored);
            assert(drawn==(fail==0)&&restored);
            D3DVIEWPORT9 actual{};DWORD enabled=0;IDirect3DSurface9 *a=nullptr,*b=nullptr;RECT sr{};
            assert(SUCCEEDED(dev->GetViewport(&actual))&&!memcmp(&actual,&vp,sizeof(vp)));
            assert(SUCCEEDED(dev->GetRenderState(D3DRS_SCISSORTESTENABLE,&enabled))&&enabled);
            assert(SUCCEEDED(dev->GetScissorRect(&sr))&&!memcmp(&sr,&sc,sizeof(sc)));
            assert(SUCCEEDED(dev->GetRenderTarget(0,&a))&&a==rt&&SUCCEEDED(dev->GetDepthStencilSurface(&b))&&b==ds);Drop(a);Drop(b);
        }
        assert(SUCCEEDED(dev->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back))&&
            SUCCEEDED(dev->CreateOffscreenPlainSurface(512,256,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&read,nullptr))&&SUCCEEDED(dev->GetRenderTargetData(back,read)));
        D3DLOCKED_RECT lock{};assert(SUCCEEDED(read->LockRect(&lock,nullptr,D3DLOCK_READONLY)));
        unsigned pixels[2]{};
        for(unsigned y=0;y<256;++y)for(unsigned x=0;x<256;++x){
            auto row=reinterpret_cast<const DWORD*>((const char*)lock.pBits+y*lock.Pitch);
            assert(row[x]==row[x+256]);if((row[x]&0xffffff)==0xc8d9e8){++pixels[0];++pixels[1];}
        }
        assert(pixels[0]>100&&pixels[0]==pixels[1]);assert(SUCCEEDED(read->UnlockRect()));
        printf("PASS own GPU startup curtain round%u pixels=%u/%u; viewport/scissor/RT/depth restored after success/failures\n",round,pixels[0],pixels[1]);
        assert(SUCCEEDED(dev->SetDepthStencilSurface(nullptr))&&SUCCEEDED(dev->SetRenderTarget(0,back)));
        Drop(read);Drop(back);Drop(ds);Drop(rt);
    }
    Drop(dev);Drop(api);FreeLibrary(dll);DestroyWindow(w);
    puts("NOT Steam/native loading/PSVR2 visual acceptance");
}
