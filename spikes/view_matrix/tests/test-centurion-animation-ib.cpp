// Own-process resource witness test, no game/Steam/input/visible window.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d9.h>
#include "../centurion_cylinder_animation.h"
#include <cstdio>
#include <cstdlib>
#pragma comment(lib,"user32.lib")
static void Check(bool yes,const char* s){if(!yes){puts(s);std::exit(1);}}
int main(){
    HWND w=CreateWindowExW(0,L"STATIC",L"private IB witness test",WS_OVERLAPPED,0,0,64,64,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    Check(w!=nullptr,"window failed");
    char path[MAX_PATH]{};Check(GetSystemDirectoryA(path,MAX_PATH)>0,"system path");strcat_s(path,"\\d3d9.dll");
    HMODULE lib=LoadLibraryA(path);Check(lib!=nullptr,"system D3D9");
    using Create=HRESULT(WINAPI*)(UINT,IDirect3D9Ex**);
    auto create=reinterpret_cast<Create>(GetProcAddress(lib,"Direct3DCreate9Ex"));Check(create!=nullptr,"D3D9Ex export");
    IDirect3D9Ex* api=nullptr;Check(SUCCEEDED(create(D3D_SDK_VERSION,&api)),"D3D9Ex");
    D3DPRESENT_PARAMETERS p{};p.BackBufferWidth=64;p.BackBufferHeight=64;p.BackBufferFormat=D3DFMT_A8R8G8B8;
    p.BackBufferCount=1;p.SwapEffect=D3DSWAPEFFECT_DISCARD;p.Windowed=TRUE;p.hDeviceWindow=w;
    IDirect3DDevice9Ex* dev=nullptr;
    Check(SUCCEEDED(api->CreateDeviceEx(0,D3DDEVTYPE_HAL,w,D3DCREATE_HARDWARE_VERTEXPROCESSING,&p,nullptr,&dev)),"create device");
    constexpr unsigned n=6767*3*2;
    for(bool enabled:{false,true}){
        const DWORD flags=centurionanim::ReadableUsage(enabled,n,true,D3DUSAGE_WRITEONLY,D3DUSAGE_WRITEONLY);
        IDirect3DIndexBuffer9* ib=nullptr;
        Check(SUCCEEDED(dev->CreateIndexBuffer(n,flags,D3DFMT_INDEX16,D3DPOOL_DEFAULT,&ib,nullptr)),"create index buffer");
        D3DINDEXBUFFER_DESC desc{};Check(SUCCEEDED(ib->GetDesc(&desc)),"description");
        Check(desc.Usage==flags&&desc.Size==n&&desc.Pool==D3DPOOL_DEFAULT,"preserved descriptor");
        void* data=nullptr;Check(SUCCEEDED(ib->Lock(0,n,&data,0))&&data,"native-like upload");
        for(unsigned i=0;i<n;++i)((unsigned char*)data)[i]=(unsigned char)(i%251);
        const auto hash=centurionanim::Hash((const unsigned char*)data,n);
        Check(SUCCEEDED(ib->Unlock()),"upload unlock");
        const HRESULT read=ib->Lock(0,n,&data,D3DLOCK_READONLY);
        printf("original WRITEONLY=%d read-only HRESULT=%08lX\n",enabled?0:1,(unsigned long)read);
        if(enabled)Check(SUCCEEDED(read)&&data,"readable candidate witness");
        if(SUCCEEDED(read)){
            Check(centurionanim::Hash((const unsigned char*)data,n)==hash,"actual byte witness roundtrip");
            Check(SUCCEEDED(ib->Unlock()),"witness unlock");
        }
        ib->Release();
    }
    dev->Release();api->Release();FreeLibrary(lib);DestroyWindow(w);
    puts("PASS own-process index upload/read witness; no Steam or animation pixel proof");
}
