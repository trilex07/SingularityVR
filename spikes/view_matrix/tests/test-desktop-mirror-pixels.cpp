// Own-process D3D9Ex pixel/RT/state/Reset fixture. No game, Steam, XR or visible UI.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../desktop_mirror.h"
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
static DWORD Pixel(IDirect3DDevice9* dev,IDirect3DSurface9* surface,UINT x,UINT y) {
    D3DSURFACE_DESC d{};HR(surface->GetDesc(&d),"read description");
    IDirect3DSurface9* sys=nullptr;
    HR(dev->CreateOffscreenPlainSurface(d.Width,d.Height,d.Format,D3DPOOL_SYSTEMMEM,&sys,nullptr),"read allocation");
    HR(dev->GetRenderTargetData(surface,sys),"pixel readback");
    D3DLOCKED_RECT lock{};HR(sys->LockRect(&lock,nullptr,D3DLOCK_READONLY),"read lock");
    const DWORD value=*(const DWORD*)((const char*)lock.pBits+y*lock.Pitch+x*4);
    HR(sys->UnlockRect(),"unlock");sys->Release();return value&0x00ffffffu;
}
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
    IDirect3DSurface9 *back=nullptr,*scene=nullptr;
    HR(dev->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back),"backbuffer");
    HR(dev->CreateRenderTarget(640,360,p.BackBufferFormat,D3DMULTISAMPLE_NONE,0,FALSE,&scene,nullptr),"offscreen scene");
    HR(dev->ColorFill(back,nullptr,0xffff0000),"red left eye");
    const RECT right{320,0,640,360};HR(dev->ColorFill(back,&right,0xff00ff00),"green right eye");
    HR(dev->ColorFill(scene,nullptr,0xff000000),"black RT0 control");
    HR(dev->SetRenderTarget(0,scene),"restore UE3-like offscreen RT0");
    const D3DVIEWPORT9 vp{7,9,600,320,.2f,.8f};HR(dev->SetViewport(&vp),"custom viewport");
    Check(desktop_mirror::Apply(dev,nullptr,nullptr,win),"compose right eye");
    Check(Pixel(dev,back,320,180)==0x00ff00,"mirror center is right eye, not black/left");
    Check(Pixel(dev,back,0,180)==0,"full eye retained with pillarbox");
    Check(Pixel(dev,scene,320,180)==0,"unrelated RT0 pixels unchanged");
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
        {desktop_mirror::PresentBackbufferScope nested(dev,true);Check(nested.Active(),"nested scope");}
    }
    IDirect3DSurface9* actual=nullptr;HR(dev->GetRenderTarget(0,&actual),"restored RT0");
    Check(actual==scene,"RT0 restored exactly");actual->Release();
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
        Check(wr.left>=mi.rcWork.left && wr.top>=mi.rcWork.top && wr.right<=mi.rcWork.right && wr.bottom<=mi.rcWork.bottom,"whole window fits work area");
    }
    g_desktopMirrorWindow=0;g_desktopMirrorEye=2;g_desktopMirrorFill=0;
    // Window resizing alone is display-only in D3D. UE3's WM_SIZE reaction is
    // deliberately NOT claimed by this own-process fixture.
    HR(dev->SetRenderTarget(0,back),"bind before reset");
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
