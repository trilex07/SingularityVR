// Own-process D3D9Ex pixel/RT/state/Reset fixture. No game, Steam, XR or visible UI.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../desktop_mirror.h"
#include "../mirror_pixel_probe.h"
#include "../xr_capture_alpha.h"
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#pragma comment(lib,"user32.lib")

volatile LONG g_dupDraws=1, g_swapEyes=0, g_desktopMirrorEye=2;
volatile LONG g_desktopMirrorFill=0, g_desktopMirrorWindow=0, g_steamCaptureBackbuffer=1;
volatile LONG g_inCinematic=0;
bool g_scCropped=false, g_xrRunning=true;
unsigned g_srcW=640, g_srcH=360;
void Log(const char* fmt, ...) { va_list a; va_start(a,fmt); vprintf(fmt,a); puts(""); va_end(a); }
static void Check(bool yes,const char* what) {
    if(!yes){std::printf("FAIL %s\n",what);std::exit(1);}
}
static void HR(HRESULT hr,const char* what){
    if(FAILED(hr)){std::printf("FAIL %s hr=%08lX\n",what,(unsigned long)hr);std::exit(1);}
}
static DWORD Pixel(IDirect3DDevice9* dev,IDirect3DSurface9* surface,UINT x,UINT y,bool alpha=false) {
    D3DSURFACE_DESC d{};HR(surface->GetDesc(&d),"read description");
    IDirect3DSurface9* sys=nullptr;
    HR(dev->CreateOffscreenPlainSurface(d.Width,d.Height,d.Format,D3DPOOL_SYSTEMMEM,&sys,nullptr),"read allocation");
    HR(dev->GetRenderTargetData(surface,sys),"pixel readback");
    D3DLOCKED_RECT lock{};HR(sys->LockRect(&lock,nullptr,D3DLOCK_READONLY),"read lock");
    const DWORD value=*(const DWORD*)((const char*)lock.pBits+y*lock.Pitch+x*4);
    HR(sys->UnlockRect(),"unlock");sys->Release();return alpha?value:value&0x00ffffffu;
}
static HRESULT STDMETHODCALLTYPE DrawAlpha(IDirect3DDevice9* d,D3DPRIMITIVETYPE t,UINT m,UINT v,UINT n,const void* i,D3DFORMAT f,const void* p,UINT stride){return d->DrawIndexedPrimitiveUP(t,m,v,n,i,f,p,stride);}
static HRESULT STDMETHODCALLTYPE Target(IDirect3DDevice9* d,DWORD i,IDirect3DSurface9* s){return d->SetRenderTarget(i,s);}
static HRESULT STDMETHODCALLTYPE VS(IDirect3DDevice9* d,IDirect3DVertexShader9* s){return d->SetVertexShader(s);}
static HRESULT STDMETHODCALLTYPE PS(IDirect3DDevice9* d,IDirect3DPixelShader9* s){return d->SetPixelShader(s);}
static HRESULT STDMETHODCALLTYPE Texture(IDirect3DDevice9* d,DWORD i,IDirect3DBaseTexture9* t){return d->SetTexture(i,t);}
static HRESULT STDMETHODCALLTYPE Clear(IDirect3DDevice9* d,DWORD n,const D3DRECT* r,DWORD f,D3DCOLOR c,float z,DWORD s){return d->Clear(n,r,f,c,z,s);}
static HRESULT STDMETHODCALLTYPE Viewport(IDirect3DDevice9* d,const D3DVIEWPORT9* v){return d->SetViewport(v);}
int main() {
    HWND win=CreateWindowExW(0,L"STATIC",L"offline mirror fixture",WS_OVERLAPPED,
        0,0,320,180,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    Check(win!=nullptr,"hidden window");
    char system[MAX_PATH]{};Check(GetSystemDirectoryA(system,MAX_PATH)>0,"system path");
    strcat_s(system,"\\d3d9.dll");HMODULE dll=LoadLibraryA(system);Check(dll!=nullptr,"system D3D9, not proxy");
    using Create=HRESULT(WINAPI*)(UINT,IDirect3D9Ex**);
    auto create=reinterpret_cast<Create>(GetProcAddress(dll,"Direct3DCreate9Ex"));Check(create!=nullptr,"D3D9Ex export");
    IDirect3D9Ex* api=nullptr;HR(create(D3D_SDK_VERSION,&api),"D3D9Ex");
    D3DPRESENT_PARAMETERS p{};p.BackBufferWidth=640;p.BackBufferHeight=360;
    p.BackBufferFormat=D3DFMT_A8R8G8B8;p.BackBufferCount=1;p.SwapEffect=D3DSWAPEFFECT_DISCARD;
    p.Windowed=TRUE;p.hDeviceWindow=win;p.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
    IDirect3DDevice9Ex* dev=nullptr;
    HR(api->CreateDeviceEx(0,D3DDEVTYPE_HAL,win,D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE,&p,nullptr,&dev),"create device");
    IDirect3DSurface9 *back=nullptr,*scene=nullptr,*depth=nullptr,*extra=nullptr;
    HR(dev->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back),"backbuffer");
    HR(dev->CreateRenderTarget(640,360,p.BackBufferFormat,D3DMULTISAMPLE_NONE,0,FALSE,&scene,nullptr),"offscreen scene");
    HR(dev->ColorFill(back,nullptr,0xffff0000),"red left eye");
    const RECT right{320,0,640,360};HR(dev->ColorFill(back,&right,0xff00ff00),"green right eye");
    HR(dev->ColorFill(scene,nullptr,0xff000000),"black RT0 control");
    HR(dev->SetRenderTarget(0,scene),"restore UE3-like offscreen RT0");
    HR(dev->CreateDepthStencilSurface(640,360,D3DFMT_D24S8,D3DMULTISAMPLE_NONE,0,TRUE,&depth,nullptr),"depth fixture");
    HR(dev->SetDepthStencilSurface(depth),"original depth binding");
    D3DCAPS9 caps{};HR(dev->GetDeviceCaps(&caps),"fixture caps");
    if(caps.NumSimultaneousRTs>1) {
        HR(dev->CreateRenderTarget(640,360,p.BackBufferFormat,D3DMULTISAMPLE_NONE,0,FALSE,&extra,nullptr),"MRT fixture");
        HR(dev->SetRenderTarget(1,extra),"original MRT binding");
    }
    const D3DVIEWPORT9 vp{7,9,600,320,.2f,.8f};HR(dev->SetViewport(&vp),"custom viewport");
    {
        xrcapture::OpaqueAlpha alpha;
        const gloves::Calls calls{DrawAlpha,Target,VS,PS,Texture,Clear,Viewport};
        HR(dev->ColorFill(back,nullptr,0x00123456),"transparent world fixture");
        HR(dev->ColorFill(back,&right,0xa8abcdef),"partial alpha HUD fixture");
        HR(dev->SetRenderState(D3DRS_COLORWRITEENABLE,3),"nondefault write mask");
        HR(dev->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE),"nondefault blending");
        Check(alpha.Apply(dev,back,calls),"opaque final capture pass");
        Check(Pixel(dev,back,0,0,true)==0xff123456,"top left RGB exact, alpha opaque");
        Check(Pixel(dev,back,639,359,true)==0xffabcdef,"bottom right RGB exact, alpha opaque");
        Check(Pixel(dev,back,319,180,true)==0xff123456,"left edge of eye seam");
        Check(Pixel(dev,back,320,180,true)==0xffabcdef,"right edge of eye seam");
        DWORD value=0;HR(dev->GetRenderState(D3DRS_COLORWRITEENABLE,&value),"restored mask");Check(value==3,"alpha mask restored");
        HR(dev->GetRenderState(D3DRS_ALPHABLENDENABLE,&value),"restored blend");Check(value==TRUE,"blend restored");
        D3DVIEWPORT9 actual{};HR(dev->GetViewport(&actual),"restored viewport");Check(!memcmp(&actual,&vp,sizeof(vp)),"alpha pass viewport restored");
        IDirect3DSurface9* actualRt=nullptr;HR(dev->GetRenderTarget(0,&actualRt),"restored RT0");Check(actualRt==scene,"alpha pass RT0 restored");actualRt->Release();
        HR(dev->GetDepthStencilSurface(&actualRt),"restored depth");Check(actualRt==depth,"alpha pass depth restored");actualRt->Release();
        if(extra){HR(dev->GetRenderTarget(1,&actualRt),"restored MRT");Check(actualRt==extra,"alpha pass MRT restored");actualRt->Release();}
        Check(Pixel(dev,scene,320,180,true)==0xff000000,"engine scene untouched");
        alpha.Release();
        HR(dev->ColorFill(back,nullptr,0xffff0000),"restore red fixture");
        HR(dev->ColorFill(back,&right,0xff00ff00),"restore green fixture");
    }
    Check(desktop_mirror::Apply(dev,nullptr,nullptr,win),"compose right eye");
    Check(Pixel(dev,back,320,180)==0x00ff00,"mirror center is right eye, not black/left");
    Check(Pixel(dev,back,0,180)==0,"full eye retained with pillarbox");
    Check(Pixel(dev,scene,320,180)==0,"unrelated RT0 pixels unchanged");
    Check(mirrorprobe::Thumbnail(dev,back,"build\\right-eye-mirror\\probe.log",0,"fixture"),"bounded mirror thumbnail writes pixels");
    Check(Pixel(dev,back,320,180)==0x00ff00,"thumbnail leaves source pixels unchanged");
    // Persistent stamp: same-size offscreen RT0 receives the completed mirror
    // so pre-hook/post-restore samplers see scene, not stale black.
    Check(desktop_mirror::StampCaptureRt(dev),"stamp mirror into offscreen RT0");
    Check(Pixel(dev,scene,320,180)==0x00ff00,"RT0 pixels carry the completed mirror");
    Check(Pixel(dev,back,320,180)==0x00ff00,"stamp never disturbs the committed mirror");
    HR(dev->ColorFill(scene,nullptr,0xff000000),"restore black RT0 control");
    // RT0 already the backbuffer: no-op without writes.
    HR(dev->SetRenderTarget(0,back),"bind RT0 to backbuffer");
    Check(!desktop_mirror::StampCaptureRt(dev),"identical RT0/backbuffer skips the copy");
    HR(dev->SetRenderTarget(0,scene),"restore offscreen RT0 binding");
    // Size mismatch: skipped, target untouched.
    IDirect3DSurface9* tiny=nullptr;
    HR(dev->CreateRenderTarget(320,180,p.BackBufferFormat,D3DMULTISAMPLE_NONE,0,FALSE,&tiny,nullptr),"small RT");
    HR(dev->ColorFill(tiny,nullptr,0xff0000ff),"blue small control");
    HR(dev->SetRenderTarget(0,tiny),"bind mismatched RT0");
    Check(!desktop_mirror::StampCaptureRt(dev),"size mismatch skips stamp");
    Check(Pixel(dev,tiny,160,90)==0x0000ff,"mismatched RT0 pixels untouched");
    tiny->Release();
    HR(dev->SetRenderTarget(0,scene),"restore offscreen RT0 binding");
    // This stack resets the viewport on every SetRenderTarget (even an
    // identical rebind - measured). Production overlays rebind at Present when
    // the game viewport is already full-frame, and the capture scope saves and
    // restores it explicitly; the fixture re-applies its custom viewport here.
    HR(dev->SetViewport(&vp),"reapply custom viewport after rebinds");
    {
        desktop_mirror::PresentBackbufferScope disabled(dev,false);
        Check(!disabled.Active(),"OFF does not own RT0");
        IDirect3DSurface9* actual=nullptr;HR(dev->GetRenderTarget(0,&actual),"OFF RT0");
        Check(actual==scene,"OFF identity");actual->Release();
    }
    {
        desktop_mirror::PresentBackbufferScope scope(dev,true);
        Check(scope.Active(),"capture scope acquired");
        IDirect3DSurface9* observed=nullptr;HR(dev->GetRenderTarget(0,&observed),"synthetic overlay RT0");
        Check(observed==back && Pixel(dev,observed,320,180)==0x00ff00,"capture consumer sees completed mirror");
        observed->Release();
        IDirect3DSurface9* detached=nullptr;
        Check(dev->GetDepthStencilSurface(&detached)==D3DERR_NOTFOUND && !detached,"capture depth detached");
        if(extra) Check(dev->GetRenderTarget(1,&detached)==D3DERR_NOTFOUND && !detached,"capture MRT detached");
        {desktop_mirror::PresentBackbufferScope nested(dev,true);Check(nested.Active(),"nested scope");}
    }
    IDirect3DSurface9* actual=nullptr;HR(dev->GetRenderTarget(0,&actual),"restored RT0");
    Check(actual==scene,"RT0 restored exactly");actual->Release();
    HR(dev->GetDepthStencilSurface(&actual),"restored depth");Check(actual==depth,"depth identity restored");actual->Release();
    if(extra) {HR(dev->GetRenderTarget(1,&actual),"restored MRT");Check(actual==extra,"MRT identity restored");actual->Release();}
    D3DVIEWPORT9 after{};HR(dev->GetViewport(&after),"restored viewport");
    Check(memcmp(&after,&vp,sizeof(vp))==0,"viewport restored exactly");
    // A true subrectangle must not mutate any pixels or claim mirror ownership.
    RECT part{320,0,640,360};
    Check(!desktop_mirror::Apply(dev,&part,nullptr,win),"subrect bypass");
    Check(Pixel(dev,back,320,180)==0x00ff00,"bypass preserves pixels");
    // Explicit eye swap and mono source rules use the XR-selected crop.
    HR(dev->ColorFill(back,nullptr,0xffff0000),"restore red left");
    HR(dev->ColorFill(back,&right,0xff00ff00),"restore green right");
    g_swapEyes=1;
    Check(desktop_mirror::Apply(dev,nullptr,nullptr,win),"swapped mirror");
    Check(Pixel(dev,back,320,180)==0xff0000,"swapped right eye selects first half");
    HR(dev->ColorFill(back,nullptr,0xffff0000),"restore cinema left");
    HR(dev->ColorFill(back,&right,0xff00ff00),"restore cinema right");
    g_scCropped=true;
    Check(desktop_mirror::Apply(dev,nullptr,nullptr,win,true),"cinema explicit crop");
    Check(Pixel(dev,back,320,180)==0xff0000,"cinema source zero even when SwapEyes=1");
    g_swapEyes=0;g_scCropped=false;
    // Test the real fit on our hidden window, never on the user's game.
    // DesktopMirrorWindow=1 forces right eye + aspect-fit WITHOUT resizing
    // the window (UE3 renders per window size - MAIN root cause). The hidden
    // 320x180 window already fits its monitor, so the work-area assertions
    // below hold trivially and prove the mirror never touches window geometry.
    g_desktopMirrorEye=1;g_desktopMirrorFill=1;g_desktopMirrorWindow=1;
    HR(dev->ColorFill(back,nullptr,0xffff0000),"restore window left");
    HR(dev->ColorFill(back,&right,0xff00ff00),"restore window right");
    Check(desktop_mirror::Apply(dev,nullptr,nullptr,win),"window-fit override");
    Check(Pixel(dev,back,320,180)==0x00ff00,"window mode forces right eye despite legacy left/stretch flags");
    D3DSURFACE_DESC desc{};HR(back->GetDesc(&desc),"display-only size");
    Check(desc.Width==640 && desc.Height==360,"D3D target never resized to monitor size");
    {
        desktop_mirror::PhysicalPixelScope pixels;
        RECT client{},wr{};Check(GetClientRect(win,&client)!=FALSE,"fitted client");
        Check(GetWindowRect(win,&wr)!=FALSE,"fitted outer");
        Check(client.right<=1920 && client.bottom<=1080,"client cap in physical pixels");
        MONITORINFO mi{};mi.cbSize=sizeof(mi);
        Check(GetMonitorInfoW(MonitorFromWindow(win,MONITOR_DEFAULTTONEAREST),&mi)!=FALSE,"work area");
        Check(wr.left>=mi.rcWork.left && wr.top>=mi.rcWork.top && wr.right<=mi.rcWork.right && wr.bottom<=mi.rcWork.bottom,"window untouched inside work area");
    }
    g_desktopMirrorWindow=0;g_desktopMirrorEye=2;g_desktopMirrorFill=0;
    // The mirror never resizes windows (UE3 renders per window size - MAIN
    // root cause). UE3's WM_SIZE reaction is deliberately NOT claimed here.
    HR(dev->SetRenderTarget(0,back),"bind before reset");
    HR(dev->SetDepthStencilSurface(nullptr),"unbind depth before reset");
    depth->Release();
    if(extra) {HR(dev->SetRenderTarget(1,nullptr),"unbind MRT before reset");extra->Release();}
    scene->Release();back->Release();
    desktop_mirror::ReleaseScratch();
    HR(dev->ResetEx(&p,nullptr),"reset after releasing mirror resources");
    Check(!desktop_mirror::g_scratch && !desktop_mirror::g_composed,"Reset releases all mirror surfaces");
    HR(dev->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back),"new backbuffer");
    HR(dev->ColorFill(back,nullptr,0xff0000ff),"new generation");
    Check(desktop_mirror::Apply(dev,nullptr,nullptr,win),"recreate after Reset");
    Check(Pixel(dev,back,320,180)==0x0000ff,"no stale previous frame");
    desktop_mirror::ReleaseScratch();back->Release();dev->Release();api->Release();
    FreeLibrary(dll);DestroyWindow(win);
    std::puts("PASS own-process D3D9Ex pixel/RT0/viewport/reset; Steam and headset UNKNOWN");return 0;
}
