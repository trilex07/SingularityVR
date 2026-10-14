// Native GPU reproduction, no Steam/game assets/input/visible window.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d9.h>
#include <cstdio>
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "user32.lib")

int main() {
    HWND window = CreateWindowExW(0, L"STATIC", L"Cinema copy test", WS_OVERLAPPED,
        0, 0, 640, 360, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (!window) return 1; // never ShowWindow
    IDirect3D9Ex* api = nullptr;
    IDirect3DDevice9Ex* dev = nullptr;
    IDirect3DSurface9 *back = nullptr, *plain = nullptr, *rt = nullptr;
    HRESULT hr = Direct3DCreate9Ex(D3D_SDK_VERSION, &api);
    D3DPRESENT_PARAMETERS p{};
    p.BackBufferWidth = 640; p.BackBufferHeight = 360; p.BackBufferFormat = D3DFMT_A8R8G8B8;
    p.BackBufferCount = 1; p.SwapEffect = D3DSWAPEFFECT_DISCARD; p.Windowed = TRUE;
    p.hDeviceWindow = window; p.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    if (SUCCEEDED(hr)) hr = api->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window,
        D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE, &p, nullptr, &dev);
    if (SUCCEEDED(hr)) hr = dev->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back);
    if (SUCCEEDED(hr)) hr = dev->CreateOffscreenPlainSurface(640,360,p.BackBufferFormat,D3DPOOL_DEFAULT,&plain,nullptr);
    if (SUCCEEDED(hr)) hr = dev->CreateRenderTarget(640,360,p.BackBufferFormat,D3DMULTISAMPLE_NONE,0,FALSE,&rt,nullptr);
    int result = 1;
    if (SUCCEEDED(hr)) {
        const HRESULT oldCopy = dev->StretchRect(back,nullptr,plain,nullptr,D3DTEXF_NONE);
        RECT fit{0,90,320,270};
        const HRESULT oldFit = dev->StretchRect(plain,nullptr,back,&fit,D3DTEXF_LINEAR);
        const HRESULT rtCopy = dev->StretchRect(back,nullptr,rt,nullptr,D3DTEXF_NONE);
        const HRESULT rtFit = dev->StretchRect(rt,nullptr,back,&fit,D3DTEXF_LINEAR);
        std::printf("Original backbuffer->plain: 0x%08lX\n", (unsigned long)oldCopy);
        std::printf("Original plain->backbuffer fit: 0x%08lX\n", (unsigned long)oldFit);
        std::printf("Backbuffer->render target: 0x%08lX\n", (unsigned long)rtCopy);
        std::printf("Render target->backbuffer fit: 0x%08lX\n", (unsigned long)rtFit);
        if ((FAILED(oldCopy) || FAILED(oldFit)) && SUCCEEDED(rtCopy) && SUCCEEDED(rtFit)) {
            std::puts("PASS unsupported original surface chain reproduced; RT chain succeeds");
            result = 0;
        } else if (SUCCEEDED(oldCopy) && SUCCEEDED(oldFit)) {
            std::puts("FALSIFIED surface type alone: original chain succeeds on this D3D9Ex driver");
            result = 0;
        }
    } else std::printf("FAIL setup: 0x%08lX\n", (unsigned long)hr);
    if (rt) rt->Release(); if (plain) plain->Release(); if (back) back->Release();
    if (dev) dev->Release(); if (api) api->Release(); DestroyWindow(window);
    return result;
}
