// Actual MAIN texture wrappers + actual injector callback. No game or XR session.
#define DllMain VrFixtureDllMainNotCalled
#include "../d3d9.cpp"
extern "C" bool FixtureHash(IDirect3DTexture9*,uint64_t*);
extern "C" bool FixtureRed(IDirect3DDevice9*,IDirect3DTexture9**);
extern "C" bool FixtureStore(IDirect3DTexture9*,IDirect3DTexture9*);
extern "C" bool FixtureHasReplacement(IDirect3DTexture9*);
extern "C" HRESULT WINAPI SingularityTextureBridgeV1(IDirect3DDevice9*,DWORD,IDirect3DBaseTexture9*,texbridge::Forward,unsigned,unsigned);
static int failed=0;
static void Check(bool ok,const char* message){printf("%s %s\n",ok?"PASS":"FAIL",message);if(!ok)++failed;}
int main(){
    InitializeCriticalSection(&g_stageLock);g_stageLockReady=true;g_textureBridgeOwned=1;g_logReady=false;
    HMODULE system=LoadLibraryExW(L"d3d9.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
    auto create=reinterpret_cast<HRESULT (WINAPI*)(UINT,IDirect3D9Ex**)>(GetProcAddress(system,"Direct3DCreate9Ex"));
    IDirect3D9Ex* api=nullptr;if(FAILED(create(D3D_SDK_VERSION,&api)))return 1;
    HWND window=CreateWindowExW(0,L"STATIC",L"Private MAIN bridge fixture",WS_POPUP,0,0,32,32,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.BackBufferWidth=32;pp.BackBufferHeight=32;pp.BackBufferFormat=D3DFMT_A8R8G8B8;pp.hDeviceWindow=window;
    IDirect3DDevice9Ex* dev=nullptr;if(FAILED(api->CreateDeviceEx(0,D3DDEVTYPE_HAL,window,D3DCREATE_SOFTWARE_VERTEXPROCESSING,&pp,nullptr,&dev)))return 1;
    g_deviceIsEx=true;g_gameDev=dev;
    g_origCreateTexture=reinterpret_cast<PFN_CreateTexture>(PatchVTable(dev,23,reinterpret_cast<void*>(&Hook_CreateTexture)));
    g_origUpdateTexture=reinterpret_cast<PFN_UpdateTexture>(PatchVTable(dev,31,reinterpret_cast<void*>(&Hook_UpdateTexture)));
    g_origSetTexture=reinterpret_cast<PFN_SetTexture>(PatchVTable(dev,65,reinterpret_cast<void*>(&Hook_SetTexture)));
    g_textureBridgeBind=&SingularityTextureBridgeV1;
    IDirect3DTexture9* source=nullptr;Check(SUCCEEDED(dev->CreateTexture(4,4,1,0,D3DFMT_DXT1,D3DPOOL_MANAGED,&source,nullptr)),"actual MAIN translated source creation");if(!source)return 1;
    D3DSURFACE_DESC desc{};source->GetLevelDesc(0,&desc);Check(desc.Pool==D3DPOOL_DEFAULT,"translated source is DEFAULT");
    D3DLOCKED_RECT lr{};Check(FAILED(g_origTexLock(source,0,&lr,nullptr,D3DLOCK_READONLY)),"raw DEFAULT LockRect fails, not assumed lockable");
    const uint8_t green[8]={0xe0,7,0xe0,7,0,0,0,0};
    Check(SUCCEEDED(source->LockRect(0,&lr,nullptr,0)),"actual wrapper redirects write to SYSTEMMEM");memcpy(lr.pBits,green,8);Check(SUCCEEDED(source->UnlockRect(0)),"actual wrapper uploads source");
    uint64_t expected=0xcbf29ce484222325ULL;for(uint8_t b:green)expected=(expected^b)*0x100000001b3ULL;
    uint64_t found=0;Check(FixtureHash(source,&found)&&found==expected,"actual injector hashes translated source through MAIN readonly lock");
    IDirect3DTexture9* replacement=nullptr;Check(FixtureRed(dev,&replacement),"actual Ex replacement factory through MAIN CreateTexture vtable");
    Check(replacement&&FixtureStore(source,replacement),"source-owned replacement");
    Check(SUCCEEDED(dev->SetTexture(1,source)),"single MAIN->callback->driver forward completes without recursion");
    IDirect3DBaseTexture9* bound=nullptr;dev->GetTexture(1,&bound);Check(bound==replacement&&shadowtrace::bound.tex[1]==source,"driver gets replacement; MAIN trace retains ORIGINAL identity");if(bound)bound->Release();dev->SetTexture(1,nullptr);
    Check(SUCCEEDED(source->LockRect(0,&lr,nullptr,0)),"source write still lockable");memcpy(lr.pBits,green,8);source->UnlockRect(0);
    Check(!FixtureHasReplacement(source),"actual MAIN write invalidates positive decision");
    Check(FixtureStore(source,replacement),"restamp before direct engine UpdateTexture");
    IDirect3DTexture9* upload=nullptr;
    Check(SUCCEEDED(dev->CreateTexture(4,4,1,0,D3DFMT_DXT1,D3DPOOL_SYSTEMMEM,&upload,nullptr)),
        "external SYSTEMMEM upload allocated");
    if(upload){
        Check(SUCCEEDED(upload->LockRect(0,&lr,nullptr,0)),"external upload lock");
        memcpy(lr.pBits,green,8);upload->UnlockRect(0);
        Check(SUCCEEDED(dev->UpdateTexture(upload,source)),"external UpdateTexture reaches MAIN hook");
        Check(!FixtureHasReplacement(source),"external UpdateTexture invalidates cached replacement");
        upload->Release();
    }
    Check(SUCCEEDED(dev->ResetEx(&pp,nullptr)),"Ex reset with actual MAIN-translated source");
    Check(FixtureHash(source,&found)&&found==expected,"source shadow/read-only hashing retained after reset");
    Check(source->Release()==0,"original not pinned; MAIN shadow teardown");
    if(replacement)Check(replacement->Release()==0,"replacement ownership released");
    unsigned liveShadows=0;for(const auto& e:g_stageTab)if(e.shadow)++liveShadows;
    Check(liveShadows==0,"no live translated shadows; cumulative allocation counter is NOT a live count");
    dev->Release();api->Release();DestroyWindow(window);FreeLibrary(system);DeleteCriticalSection(&g_stageLock);return failed?1:0;
}
