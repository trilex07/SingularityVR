// Offline only: exact static target table, system D3D9 synthetic resources.
// Not a Steam FPS benchmark. No installed mod, DDS files or INI are used.
#include "../resource_native_cache.h"
#include <d3d9.h>
#include <cstdio>
#include <vector>
struct Target {const char* name; UINT w,h; D3DFORMAT fmt; uint64_t hash; const char* dds; const char* group;};
#include "../targets_generated.h"
static __declspec(noinline) bool IsCandidate(const D3DSURFACE_DESC& d) {
    for(int i=0;i<kTargetCount;++i)
        if(kTargets[i].w==d.Width && kTargets[i].h==d.Height && kTargets[i].fmt==d.Format)return true;
    return false;
}
int main() {
    HMODULE library=LoadLibraryExW(L"d3d9.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
    auto create=library?reinterpret_cast<IDirect3D9* (WINAPI*)(UINT)>(GetProcAddress(library,"Direct3DCreate9")):nullptr;
    IDirect3D9* d3d=create?create(D3D_SDK_VERSION):nullptr;if(!d3d)return 1;
    HWND window=CreateWindowExW(0,L"STATIC",L"Offline shape timing",WS_POPUP,0,0,32,32,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;
    pp.hDeviceWindow=window;pp.BackBufferWidth=32;pp.BackBufferHeight=32;
    IDirect3DDevice9* device=nullptr;
    if(FAILED(d3d->CreateDevice(0,D3DDEVTYPE_HAL,window,D3DCREATE_SOFTWARE_VERTEXPROCESSING,&pp,&device)))return 1;
    std::vector<IDirect3DTexture9*> textures;
    for(unsigned n=0;n<64;++n) {
        IDirect3DTexture9* texture=nullptr;
        if(FAILED(device->CreateTexture(32,32,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&texture,nullptr)))return 1;
        textures.push_back(texture);
    }
    LARGE_INTEGER frequency{};QueryPerformanceFrequency(&frequency);
    constexpr unsigned calls=2000000;
    volatile unsigned valid=0;
    double oldUs[5]{},newUs[5]{};
    // First cold pass plus four warm passes; QPC outside the loops only.
    for(unsigned round=0;round<5;++round) {
        LARGE_INTEGER start{},end{};QueryPerformanceCounter(&start);
        for(unsigned n=0;n<calls;++n) {
            auto* texture=textures[n%textures.size()];
            const bool stamped=texnative::KnownNative(texture);
            D3DSURFACE_DESC desc{};
            valid+=!stamped && SUCCEEDED(texture->GetLevelDesc(0,&desc)) && !IsCandidate(desc);
        }
        QueryPerformanceCounter(&end);oldUs[round]=double(end.QuadPart-start.QuadPart)*1e6/frequency.QuadPart/calls;
        // Do not retain stamps for the following old-path pass.
        for(auto* texture:textures)if(!texnative::RememberNative(texture))return 1;
        QueryPerformanceCounter(&start);
        for(unsigned n=0;n<calls;++n)valid+=texnative::KnownNative(textures[n%textures.size()]);
        QueryPerformanceCounter(&end);newUs[round]=double(end.QuadPart-start.QuadPart)*1e6/frequency.QuadPart/calls;
        for(auto* texture:textures)if(FAILED(texture->FreePrivateData(texnative::nativeDecisionId)))return 1;
        std::printf("SYNTHETIC round=%u calls=%u noncandidate_us=%.6f stamped_us=%.6f delta_us=%.6f (not game FPS)\n",round,calls,oldUs[round],newUs[round],oldUs[round]-newUs[round]);
    }
    const bool ok=valid==calls*10;
    for(auto* texture:textures)texture->Release();device->Release();DestroyWindow(window);d3d->Release();FreeLibrary(library);
    std::printf("%s exact %d-target noncandidate classifications; no game assets read\n",ok?"PASS":"FAIL",kTargetCount);
    return ok?0:1;
}
