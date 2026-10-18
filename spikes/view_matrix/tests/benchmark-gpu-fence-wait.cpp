#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d9.h>
#include <cstdio>
#include "support/gpu_fence_wait.h"
struct Stats{double wall=0,cpu=0;unsigned frames=0,failed=0;unsigned long long polls=0,flushes=0,yields=0,switched=0,cycles=0;};
static unsigned long long Cpu(){FILETIME c{},e{},k{},u{};GetThreadTimes(GetCurrentThread(),&c,&e,&k,&u);
    return ((unsigned long long)k.dwHighDateTime<<32)+k.dwLowDateTime+((unsigned long long)u.dwHighDateTime<<32)+u.dwLowDateTime;}
int main(){
    wchar_t path[MAX_PATH]{};GetSystemDirectoryW(path,MAX_PATH);wcscat_s(path,L"\\d3d9.dll");
    HMODULE library=LoadLibraryW(path);if(!library)return 1;
    using Create=HRESULT(WINAPI*)(UINT,IDirect3D9Ex**);auto create=(Create)GetProcAddress(library,"Direct3DCreate9Ex");
    IDirect3D9Ex* api=nullptr;if(!create||FAILED(create(D3D_SDK_VERSION,&api)))return 1;
    HWND window=CreateWindowW(L"STATIC",L"fence bench",WS_OVERLAPPED,0,0,32,32,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.hDeviceWindow=window;
    pp.BackBufferWidth=1280;pp.BackBufferHeight=720;pp.BackBufferFormat=D3DFMT_A8R8G8B8;pp.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
    IDirect3DDevice9Ex* dev=nullptr;IDirect3DQuery9* fence=nullptr;
    if(FAILED(api->CreateDeviceEx(0,D3DDEVTYPE_HAL,window,D3DCREATE_SOFTWARE_VERTEXPROCESSING,&pp,nullptr,&dev))||
       FAILED(dev->CreateQuery(D3DQUERYTYPE_EVENT,&fence)))return 1;
    LARGE_INTEGER hz{};QueryPerformanceFrequency(&hz);Stats stats[3];
    // ABCCBA,40frames/block, identical workload; coarse CPU times + CPU cycles.
    const unsigned order[]={0,1,2,2,1,0};
    for(unsigned block:order){bool cooperative=block==1;auto& s=stats[block];
        for(unsigned frame=0;frame<40;++frame){
            dev->BeginScene();for(unsigned pass=0;pass<2048;++pass)dev->Clear(0,nullptr,D3DCLEAR_TARGET,0xff123456+pass,1,0);dev->EndScene();
            const auto cpu=Cpu();ULONG64 c0=0,c1=0;QueryThreadCycleTime(GetCurrentThread(),&c0);
            LARGE_INTEGER start{},end{};QueryPerformanceCounter(&start);
            if(FAILED(fence->Issue(D3DISSUE_END)))return 1;BOOL done=FALSE;
            auto result=gpufence::Wait(cooperative,[&](bool flush){return fence->GetData(&done,sizeof(done),flush?D3DGETDATA_FLUSH:0);},
                [&]{LARGE_INTEGER now{};QueryPerformanceCounter(&now);return double(now.QuadPart-start.QuadPart)*1000/hz.QuadPart;},
                []{YieldProcessor();},[]{return SwitchToThread()!=FALSE;},block==2);
            QueryPerformanceCounter(&end);s.wall+=double(end.QuadPart-start.QuadPart)*1000/hz.QuadPart;
            QueryThreadCycleTime(GetCurrentThread(),&c1);s.cycles+=c1-c0;
            s.cpu+=double(Cpu()-cpu)/10000.;++s.frames;s.polls+=result.polls;s.flushes+=result.flushes;s.yields+=result.yields;s.switched+=result.switched;
            if(result.timeout||result.status!=S_OK||!done)++s.failed;
            dev->Present(nullptr,nullptr,nullptr,nullptr);
        }
    }
    const char* names[]={"spin","cooperative","coalesced-flush"};
    for(unsigned i=0;i<3;++i){auto& s=stats[i];printf("fence synthetic %s frames=%u wall_ms=%.4f thread_cpu_ms=%.4f cycles/frame=%llu polls=%llu flushes=%llu yields=%llu switched=%llu failures=%u\n",
        names[i],s.frames,s.wall/s.frames,s.cpu/s.frames,s.cycles/s.frames,s.polls,s.flushes,s.yields,s.switched,s.failed);}
    fence->Release();dev->Release();api->Release();DestroyWindow(window);FreeLibrary(library);
    return stats[0].failed||stats[1].failed||stats[2].failed?1:0;
}
