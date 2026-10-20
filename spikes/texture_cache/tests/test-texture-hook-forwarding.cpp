// Exercise the ACTUAL hook against native synthetic resources, no game/DDS/INI.
#define TEXCACHE_TIMING 1
#include "../texture_injector.cpp"
static int failures=0;
static unsigned forwarded=0;
static IDirect3DBaseTexture9* lastTexture=nullptr;
static DWORD lastStage=99;
static void Check(bool ok,const char* message){std::printf("%s %s\n",ok?"PASS":"FAIL",message);if(!ok)++failures;}
static HRESULT STDMETHODCALLTYPE Forward(IDirect3DDevice9* dev,DWORD stage,IDirect3DBaseTexture9* texture) {
    ++forwarded;lastStage=stage;lastTexture=texture;
    dev->SetTexture(stage,texture);
    return E_ACCESSDENIED; // deliberately keep failing HRESULT, not S_OK
}
static void Exercise(IDirect3DDevice9* dev,IDirect3DTexture9* input,IDirect3DTexture9* expected,const char* label) {
    const unsigned before=forwarded;
    const HRESULT hr=HookSetTexture(dev,1,input);
    IDirect3DBaseTexture9* bound=nullptr;dev->GetTexture(1,&bound);
    Check(hr==E_ACCESSDENIED && forwarded==before+1 && lastStage==1 && lastTexture==expected && bound==expected,label);
    if(bound)bound->Release();
}
int main() {
    HMODULE system9=LoadLibraryExW(L"d3d9.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
    auto create=system9?reinterpret_cast<IDirect3D9* (WINAPI*)(UINT)>(GetProcAddress(system9,"Direct3DCreate9")):nullptr;
    auto createEx=system9?reinterpret_cast<HRESULT (WINAPI*)(UINT,IDirect3D9Ex**)>(GetProcAddress(system9,"Direct3DCreate9Ex")):nullptr;
    IDirect3D9* d3d=create?create(D3D_SDK_VERSION):nullptr;
    IDirect3D9Ex* d3dEx=nullptr;
    if(createEx)createEx(D3D_SDK_VERSION,&d3dEx);
    if(!d3d && !d3dEx)return 1;
    HWND window=CreateWindowExW(0,L"STATIC",L"Private hook test",WS_POPUP,0,0,32,32,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;
    pp.hDeviceWindow=window;pp.BackBufferWidth=32;pp.BackBufferHeight=32;pp.BackBufferFormat=D3DFMT_A8R8G8B8;
    IDirect3DDevice9* dev=nullptr;
    IDirect3DDevice9Ex* devEx=nullptr;
    if(d3dEx){
        if(SUCCEEDED(d3dEx->CreateDeviceEx(0,D3DDEVTYPE_HAL,window,D3DCREATE_SOFTWARE_VERTEXPROCESSING,&pp,nullptr,&devEx)))dev=devEx;
    }
    if(!dev && d3d){
        if(FAILED(d3d->CreateDevice(0,D3DDEVTYPE_HAL,window,D3DCREATE_SOFTWARE_VERTEXPROCESSING,&pp,&dev)))return 1;
    }
    if(!dev)return 1;
    IDirect3DTexture9* textures[4]{};
    const D3DPOOL pool=devEx?D3DPOOL_DEFAULT:D3DPOOL_MANAGED;
    for(auto& texture:textures)if(FAILED(dev->CreateTexture(32,32,1,0,D3DFMT_A8R8G8B8,pool,&texture,nullptr)))return 1;
    RealSetTexture=Forward;g_textureTiming.Start();
    g_enabled=false;Exercise(dev,textures[0],textures[0],"disabled actual hook preserves pointer/stage/HRESULT/exact forward count");
    g_enabled=true;Exercise(dev,nullptr,nullptr,"null binding preserved");
    Exercise(dev,textures[0],textures[0],"noncandidate is still native without adding stamp");
    Check(!texnative::KnownNative(textures[0]),"noncandidate policy not changed by instrumentation");
    Check(texnative::RememberNative(textures[0]),"synthetic already-native result stamped");
    Exercise(dev,textures[0],textures[0],"native metadata hit forwards original");
    g_cacheCount=2;
    g_cache[0].tex=textures[1];g_cache[0].resolved=true;
    g_cache[1].tex=textures[2];g_cache[1].resolved=true;g_cache[1].replacement=textures[3];
    Exercise(dev,textures[1],textures[1],"legacy native policy unchanged");
    Exercise(dev,textures[2],textures[3],"positive replacement still bound, original stage and failing HRESULT preserved");
    const auto before=forwarded;
    for(unsigned n=0;n<20000;++n)HookSetTexture(dev,1,textures[0]);
    Check(forwarded==before+20000 && g_textureTiming.Read(textiming::Route::native_hit).samples>0,"actual native-hit hook produces bounded sparse samples with no dropped binds");
    g_textureTiming.Stop();dev->SetTexture(1,nullptr);
    g_cache[0]=Entry{};g_cache[1]=Entry{};g_cacheCount=0;
    for(auto* texture:textures)texture->Release();dev->Release();DestroyWindow(window);if(d3d)d3d->Release();if(d3dEx)d3dEx->Release();FreeLibrary(system9);
    return failures?1:0;
}
