#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d9.h>
#include <cstdio>
#include "../frame_gpu_probe.h"

// Standalone, hidden32x32 D3D9Ex fixture. No Steam/game/OpenXR or installed files.
// Metadata below is synthetic; this validates query contracts, not game FPS.
int main(){
    wchar_t path[MAX_PATH]{};GetSystemDirectoryW(path,MAX_PATH);wcscat_s(path,L"\\d3d9.dll");
    HMODULE library=LoadLibraryW(path);if(!library)return 1;
    using Create=HRESULT(WINAPI*)(UINT,IDirect3D9Ex**);
    auto create=reinterpret_cast<Create>(GetProcAddress(library,"Direct3DCreate9Ex"));
    IDirect3D9Ex* api=nullptr;if(!create||FAILED(create(D3D_SDK_VERSION,&api))){FreeLibrary(library);return 1;}
    HWND window=CreateWindowW(L"STATIC",L"SingularityVR GPU probe fixture",WS_OVERLAPPED,0,0,32,32,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;
    pp.BackBufferWidth=32;pp.BackBufferHeight=32;pp.BackBufferFormat=D3DFMT_A8R8G8B8;
    pp.hDeviceWindow=window;pp.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
    IDirect3DDevice9Ex* device=nullptr;
    if(!window||FAILED(api->CreateDeviceEx(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,window,D3DCREATE_SOFTWARE_VERTEXPROCESSING,&pp,nullptr,&device))){
        if(window)DestroyWindow(window);api->Release();FreeLibrary(library);return 1;}
    framegpu::Probe<IDirect3DDevice9,IDirect3DQuery9> probe;
    probe.CapturePasses(true);
    const bool supported=probe.Prepare(device);
    // Hidden dummy windows can be occluded, so Present need not submit work.
    // Fixture-only flush requests drive the queue; the production probe never
    // flushes or waits (the game's existing XR-copy path already submits).
    IDirect3DQuery9* fixtureSubmit=nullptr;device->CreateQuery(D3DQUERYTYPE_EVENT,&fixtureSubmit);
    DWORD cullBefore=0,cullAfter=0;device->GetRenderState(D3DRS_CULLMODE,&cullBefore);
    double instrumentCpuMs=0;LARGE_INTEGER clock{};QueryPerformanceFrequency(&clock);
    if(supported)for(unsigned frame=0;frame<240;++frame){
        if(FAILED(device->BeginScene()))break;
        LARGE_INTEGER t0{},t1{};QueryPerformanceCounter(&t0);
        if(frame<80)probe.Begin(frame);
        probe.Draw(1,false);
        device->Clear(0,nullptr,D3DCLEAR_TARGET,0xff123456,1,0);
        probe.Target(2); // synthetic boundary identity, no actual RT/state mutation
        probe.Draw(2,true);
        device->Clear(0,nullptr,D3DCLEAR_TARGET,0xff334455,1,0);
        device->EndScene();probe.End(true,1,11.1,1000.0/90);
        QueryPerformanceCounter(&t1);instrumentCpuMs+=double(t1.QuadPart-t0.QuadPart)*1000/clock.QuadPart;
        device->Present(nullptr,nullptr,nullptr,nullptr);
        if(fixtureSubmit){fixtureSubmit->Issue(D3DISSUE_END);fixtureSubmit->GetData(nullptr,0,D3DGETDATA_FLUSH);}
        Sleep(1);QueryPerformanceCounter(&t0);probe.Poll();QueryPerformanceCounter(&t1);
        instrumentCpuMs+=double(t1.QuadPart-t0.QuadPart)*1000/clock.QuadPart;
    }
    device->GetRenderState(D3DRS_CULLMODE,&cullAfter);
    const unsigned samples=probe.within.samples+probe.over.samples;
    std::printf("D3D9Ex fixture: supported=%d issued=%u samples=%u skipped=%u invalid=%u failures=%u stateUnchanged=%d (synthetic scene, not Steam FPS)\n",
        supported?1:0,probe.issued,samples,probe.skipped,probe.invalid,probe.failed,cullBefore==cullAfter?1:0);
    const auto& sample=probe.within.samples?probe.within.worstWall:probe.over.worstWall;
    const double partition=sample.passes[0].gpuMs+sample.passes[1].gpuMs;
    std::printf("pass fixture: complete=%d phases=%u sumMatches=%d\n",sample.passesComplete?1:0,sample.passCount,std::abs(partition-sample.gpuMs)<1e-9?1:0);
    std::printf("fixture CPU probe+two clears+EndScene+poll average=%.6f ms/frame (NOT game overhead or GPU busy time)\n",instrumentCpuMs/240);
    const bool ok=supported&&samples>0&&!probe.failed&&cullBefore==cullAfter&&sample.passesComplete&&sample.passCount==2&&std::abs(partition-sample.gpuMs)<1e-9;
    probe.Release();if(fixtureSubmit)fixtureSubmit->Release();device->Release();api->Release();DestroyWindow(window);FreeLibrary(library);
    return ok?0:supported?1:2;
}
