#include "../resource_native_cache.h"
#include <d3d9.h>
#include <cstdio>
#include <vector>
static int failures=0;
static void Check(bool ok,const char* text){std::printf("%s %s\n",ok?"PASS":"FAIL",text);if(!ok)++failures;}
static bool Pixels(IDirect3DTexture9* texture,DWORD value,bool write) {
    D3DLOCKED_RECT lock{};
    if(FAILED(texture->LockRect(0,&lock,nullptr,write?0:D3DLOCK_READONLY)))return false;
    bool equal=true;
    for(unsigned y=0;y<4;++y)for(unsigned x=0;x<4;++x){
        auto* pixel=reinterpret_cast<DWORD*>(static_cast<unsigned char*>(lock.pBits)+y*lock.Pitch)+x;
        if(write)*pixel=value;else if(*pixel!=value)equal=false;
    }
    const HRESULT hr=texture->UnlockRect(0);return equal && SUCCEEDED(hr);
}
int main(){
    HMODULE system9=LoadLibraryExW(L"d3d9.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
    Check(system9!=nullptr,"system D3D9 loaded without installed mod proxy");if(!system9)return 1;
    auto create=reinterpret_cast<IDirect3D9* (WINAPI*)(UINT)>(GetProcAddress(system9,"Direct3DCreate9"));
    IDirect3D9* d3d=create?create(D3D_SDK_VERSION):nullptr;
    Check(d3d!=nullptr,"native D3D9 interface");if(!d3d)return 1;
    HWND window=CreateWindowExW(0,L"STATIC",L"Private cache test",WS_POPUP,0,0,32,32,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;
    pp.hDeviceWindow=window;pp.BackBufferWidth=32;pp.BackBufferHeight=32;pp.BackBufferFormat=D3DFMT_UNKNOWN;
    IDirect3DDevice9* device=nullptr;
    HRESULT hr=d3d->CreateDevice(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,window,D3DCREATE_SOFTWARE_VERTEXPROCESSING,&pp,&device);
    Check(SUCCEEDED(hr) && device,"hidden synthetic native device");if(!device){DestroyWindow(window);d3d->Release();return 1;}
    std::vector<IDirect3DTexture9*> textures;
    bool all=true;
    for(unsigned i=0;i<655;++i){
        IDirect3DTexture9* texture=nullptr;
        hr=device->CreateTexture(4,4,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&texture,nullptr);
        if(FAILED(hr)||!texture){all=false;break;}
        textures.push_back(texture);
        if(texnative::KnownNative(texture) || !Pixels(texture,0xff123400u+i,true) ||
           !texnative::RememberNative(texture) || !texnative::KnownNative(texture) ||
           !Pixels(texture,0xff123400u+i,false)){all=false;break;}
    }
    Check(all && textures.size()==655,"655 real resources keep decisions and identical mip0 pixels");
    if(!textures.empty()){
        device->SetTexture(0,textures[0]);
        texnative::KnownNative(textures[0]);texnative::RememberNative(textures[0]);
        IDirect3DBaseTexture9* bound=nullptr;device->GetTexture(0,&bound);
        Check(bound==textures[0],"metadata leaves bound texture/state unchanged");if(bound)bound->Release();
        device->SetTexture(0,nullptr);
    }
    hr=device->Reset(&pp);
    Check(SUCCEEDED(hr),"synthetic device reset");
    all=true;
    for(unsigned i=0;i<textures.size();++i)
        if(!texnative::KnownNative(textures[i]) || !Pixels(textures[i],0xff123400u+i,false))all=false;
    Check(all,"managed resource decisions/pixels retained across reset");
    for(auto* texture:textures)texture->Release();
    IDirect3DTexture9* fresh=nullptr;
    hr=device->CreateTexture(4,4,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&fresh,nullptr);
    Check(SUCCEEDED(hr) && fresh && !texnative::KnownNative(fresh),"released resources cannot stamp a fresh lifetime");
    if(fresh)fresh->Release();device->Release();DestroyWindow(window);d3d->Release();FreeLibrary(system9);
    return failures?1:0;
}
