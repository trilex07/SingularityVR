#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "../cinema_surface_copy.h"
#include <cstdio>
#include <cstring>
#pragma comment(lib,"d3d9.lib")
#pragma comment(lib,"user32.lib")
static int failures=0;
static void Check(bool ok,const char* msg){std::printf("%s %s\n",ok?"PASS":"FAIL",msg);if(!ok)++failures;}
static HRESULT STDMETHODCALLTYPE SetTarget(IDirect3DDevice9* d,DWORD i,IDirect3DSurface9* s){return d->SetRenderTarget(i,s);}
static HRESULT STDMETHODCALLTYPE Clear(IDirect3DDevice9* d,DWORD n,const D3DRECT* r,DWORD f,D3DCOLOR c,float z,DWORD s){return d->Clear(n,r,f,c,z,s);}
static HRESULT STDMETHODCALLTYPE RefuseClear(IDirect3DDevice9* d,DWORD n,const D3DRECT* r,DWORD f,D3DCOLOR c,float z,DWORD s){
    // Simulate a failed operation after pixels were already touched.
    const HRESULT hr=d->Clear(n,r,f,c,z,s);
    return FAILED(hr)?hr:D3DERR_INVALIDCALL;
}
static bool Pixels(IDirect3DDevice9* d,IDirect3DSurface9* back,const D3DSURFACE_DESC& desc,const CutsceneMonoFit& fit,bool composed){
    IDirect3DSurface9* sys=nullptr;
    if(FAILED(d->CreateOffscreenPlainSurface(desc.Width,desc.Height,desc.Format,D3DPOOL_SYSTEMMEM,&sys,nullptr)))return false;
    bool ok=SUCCEEDED(d->GetRenderTargetData(back,sys));
    D3DLOCKED_RECT lock{};
    if(ok)ok=SUCCEEDED(sys->LockRect(&lock,nullptr,D3DLOCK_READONLY));
    if(ok){
        const DWORD colours[]={0xFFFF0000,0xFF00FF00,0xFF0000FF,0xFFFFFF00};
        const int iw=fit.image.right-fit.image.left,ih=fit.image.bottom-fit.image.top;
        for(int i=0;i<4;++i){
            int x=composed?fit.image.left+iw*((i%2)?3:1)/4:(int)desc.Width*((i%2)?3:1)/4;
            int y=composed?fit.image.top+ih*((i/2)?3:1)/4:(int)desc.Height*((i/2)?3:1)/4;
            const DWORD value=*reinterpret_cast<const DWORD*>(static_cast<const unsigned char*>(lock.pBits)+y*lock.Pitch+x*4);
            ok=ok&&value==colours[i]; // includes opaque alpha
        }
        if(composed){
            const DWORD bar=*reinterpret_cast<const DWORD*>(lock.pBits);
            ok=ok&&bar==0xFF000000;
        }
        sys->UnlockRect();
    }
    sys->Release();return ok;
}
static void Run(UINT w,UINT h){
    HWND window=CreateWindowExW(0,L"STATIC",L"Synthetic cinema",WS_OVERLAPPED,0,0,640,360,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    IDirect3D9Ex* api=nullptr;IDirect3DDevice9Ex* dev=nullptr;
    IDirect3DSurface9 *back=nullptr,*saved=nullptr,*aux=nullptr;IDirect3DTexture9* texture=nullptr;
    HRESULT hr=Direct3DCreate9Ex(D3D_SDK_VERSION,&api);
    D3DPRESENT_PARAMETERS p{};p.Windowed=TRUE;p.hDeviceWindow=window;p.BackBufferWidth=w;p.BackBufferHeight=h;
    p.BackBufferFormat=D3DFMT_A8R8G8B8;p.BackBufferCount=1;p.SwapEffect=D3DSWAPEFFECT_DISCARD;p.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
    if(SUCCEEDED(hr))hr=api->CreateDeviceEx(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,window,D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE,&p,nullptr,&dev);
    if(SUCCEEDED(hr))hr=dev->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back);
    D3DSURFACE_DESC desc{};
    if(SUCCEEDED(hr))hr=back->GetDesc(&desc);
    if(SUCCEEDED(hr))hr=CreateCinemaCopyTarget(dev,desc,&texture,&saved);
    Check(SUCCEEDED(hr),"production scratch RT created");
    if(SUCCEEDED(hr)){
        const DWORD colours[]={0xFFFF0000,0xFF00FF00,0xFF0000FF,0xFFFFFF00};
        for(int i=0;i<4;++i){D3DRECT r{(LONG)(i%2*w/2),(LONG)(i/2*h/2),(LONG)((i%2+1)*w/2),(LONG)((i/2+1)*h/2)};Check(SUCCEEDED(dev->Clear(1,&r,D3DCLEAR_TARGET,colours[i],1,0)),"synthetic source quadrant");}
        D3DSURFACE_DESC targetDesc{};saved->GetDesc(&targetDesc);
        Check(targetDesc.Pool==D3DPOOL_DEFAULT&&(targetDesc.Usage&D3DUSAGE_RENDERTARGET)&&targetDesc.Format==desc.Format,"target type/pool/format contract");
        D3DVIEWPORT9 before{7,13,w/3,h/3,.2f,.9f};dev->SetViewport(&before);dev->SetRenderState(D3DRS_SCISSORTESTENABLE,TRUE);
        const auto fit=CutsceneMonoComputeFit(w,h,0,w/2,h,85);
        D3DCAPS9 caps{};dev->GetDeviceCaps(&caps);
        if(caps.NumSimultaneousRTs>1) {
            Check(SUCCEEDED(dev->CreateRenderTarget(w,h,desc.Format,D3DMULTISAMPLE_NONE,0,FALSE,&aux,nullptr)),"auxiliary MRT created");
            if(aux) {
                Check(SUCCEEDED(dev->ColorFill(aux,nullptr,0xFFFF0000)),"auxiliary MRT sentinel filled");
                Check(SUCCEEDED(dev->SetRenderTarget(1,aux)),"auxiliary MRT bound");
            }
        }
        const auto refused=ComposeCinemaSurface(dev,back,saved,desc,fit,SetTarget,RefuseClear);
        Check(FAILED(refused.hr)&&refused.stateRestored&&refused.pixelsRestored,"injected clear failure returns safe fallback");
        Check(Pixels(dev,back,desc,fit,false),"failed composition leaves original RGB/alpha intact");
        const auto result=ComposeCinemaSurface(dev,back,saved,desc,fit,SetTarget,Clear);
        std::printf("%ux%u compose=%s hr=0x%08lX\n",w,h,result.stage,(unsigned long)result.hr);
        Check(SUCCEEDED(result.hr)&&result.stateRestored,"production composition helper succeeds");
        Check(Pixels(dev,back,desc,fit,true),"whole source quadrants RGB/alpha in left fit, opaque black bars");
        D3DVIEWPORT9 after{};DWORD scissor=FALSE;dev->GetViewport(&after);dev->GetRenderState(D3DRS_SCISSORTESTENABLE,&scissor);
        IDirect3DSurface9* restored=nullptr;dev->GetRenderTarget(0,&restored);
        Check(std::memcmp(&before,&after,sizeof(before))==0&&scissor==TRUE&&restored==back,"viewport/scissor/target restored exactly");
        if(restored)restored->Release();
        if(aux) {
            IDirect3DSurface9* extraAfter=nullptr;dev->GetRenderTarget(1,&extraAfter);
            Check(extraAfter==aux,"auxiliary MRT identity restored");
            if(extraAfter)extraAfter->Release();
            IDirect3DSurface9* sysAux=nullptr;
            bool untouched=SUCCEEDED(dev->CreateOffscreenPlainSurface(w,h,desc.Format,D3DPOOL_SYSTEMMEM,&sysAux,nullptr));
            D3DLOCKED_RECT auxLock{};
            if(untouched)untouched=SUCCEEDED(dev->GetRenderTargetData(aux,sysAux));
            if(untouched)untouched=SUCCEEDED(sysAux->LockRect(&auxLock,nullptr,D3DLOCK_READONLY));
            if(untouched) {
                untouched=*static_cast<const DWORD*>(auxLock.pBits)==0xFFFF0000;
                sysAux->UnlockRect();
            }
            Check(untouched,"backbuffer matte did not clear auxiliary MRT pixels");
            if(sysAux)sysAux->Release();
        }
        Check(CreateCinemaCopyTarget(nullptr,desc,&texture,&saved)==E_INVALIDARG,"invalid device rejected without discarding owned resources");
    }else std::printf("setup hr=0x%08lX\n",(unsigned long)hr);
    if(aux)aux->Release();if(saved)saved->Release();if(texture)texture->Release();if(back)back->Release();if(dev)dev->Release();if(api)api->Release();if(window)DestroyWindow(window);
}
int main(){Run(640,360);Run(5264,2684);return failures?1:0;}
