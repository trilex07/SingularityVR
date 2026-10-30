// Actual callback/factory + resource lifetime on native hidden plain/Ex devices.
// Synthetic DXT blocks only: no game, assets, Steam, external config or proxy.
#include "../texture_injector.cpp"
static int failures=0;static unsigned calls=0;
static void Check(bool ok,const char* text){printf("%s %s\n",ok?"PASS":"FAIL",text);if(!ok)++failures;}
static HRESULT STDMETHODCALLTYPE Forward(IDirect3DDevice9* d,DWORD s,IDirect3DBaseTexture9* t){++calls;d->SetTexture(s,t);return E_ACCESSDENIED;}
static bool ReadPixel(IDirect3DDevice9* dev,IDirect3DTexture9* tex,DWORD expected){
    dev->SetTexture(0,tex);dev->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1);
    dev->SetRenderState(D3DRS_ZENABLE,FALSE);dev->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
    dev->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);dev->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);
    dev->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);dev->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);
    dev->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);
    struct V{float x,y,z,w,u,v;} v[4]={{-.5f,-.5f,0,1,0,0},{31.5f,-.5f,0,1,1,0},{-.5f,31.5f,0,1,0,1},{31.5f,31.5f,0,1,1,1}};
    HRESULT hr=dev->BeginScene();if(FAILED(hr))return false;hr=dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,v,sizeof(V));dev->EndScene();if(FAILED(hr))return false;
    IDirect3DSurface9 *back=nullptr,*cpu=nullptr;bool ok=false;
    if(SUCCEEDED(dev->GetRenderTarget(0,&back))&&SUCCEEDED(dev->CreateOffscreenPlainSurface(32,32,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&cpu,nullptr))&&
       SUCCEEDED(dev->GetRenderTargetData(back,cpu))){D3DLOCKED_RECT lr{};if(SUCCEEDED(cpu->LockRect(&lr,nullptr,D3DLOCK_READONLY))){
        DWORD pixel=*reinterpret_cast<DWORD*>(static_cast<char*>(lr.pBits)+16*lr.Pitch+16*4);ok=(pixel&0xffffff)==(expected&0xffffff)&&(expected<=0xffffff||(pixel>>24)==(expected>>24));cpu->UnlockRect();}}
    if(cpu)cpu->Release();if(back)back->Release();dev->SetTexture(0,nullptr);return ok;
}
static void Run(IDirect3D9* api,IDirect3D9Ex* apiEx,bool ex){
    HWND window=CreateWindowExW(0,L"STATIC",L"Private texture bridge",WS_POPUP,0,0,32,32,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.BackBufferWidth=32;pp.BackBufferHeight=32;pp.BackBufferFormat=D3DFMT_A8R8G8B8;pp.hDeviceWindow=window;
    IDirect3DDevice9* dev=nullptr;IDirect3DDevice9Ex* devEx=nullptr;
    HRESULT hr=ex?apiEx->CreateDeviceEx(0,D3DDEVTYPE_HAL,window,D3DCREATE_SOFTWARE_VERTEXPROCESSING,&pp,nullptr,&devEx):api->CreateDevice(0,D3DDEVTYPE_HAL,window,D3DCREATE_SOFTWARE_VERTEXPROCESSING,&pp,&dev);
    if(ex)dev=devEx;Check(SUCCEEDED(hr)&&dev,ex?"Ex device":"plain device");if(!dev){DestroyWindow(window);return;}
    g_bridgeOwned=true;g_enabled=true;
    DDSImage image;image.w=image.h=4;image.levels=1;image.fmt=D3DFMT_DXT1;image.mip={{0x00,0xf8,0x00,0xf8,0,0,0,0}};
    IDirect3DTexture9 *replacement=nullptr,*source=nullptr;
    Check(CreateReplacement(dev,image,&replacement),"factory creates/uploads synthetic red DXT1");
    if(!replacement){dev->Release();DestroyWindow(window);return;}
    D3DSURFACE_DESC desc{};replacement->GetLevelDesc(0,&desc);
    Check(desc.Pool==(ex?D3DPOOL_DEFAULT:D3DPOOL_MANAGED),"actual pool is Ex DEFAULT / plain MANAGED");
    Check(ReadPixel(dev,replacement,0xff0000),"readback proves uploaded RGB (not black)");
    DDSImage rgba;rgba.w=rgba.h=4;rgba.levels=1;rgba.fmt=D3DFMT_DXT5;
    rgba.mip={{64,64,0,0,0,0,0,0,0xe0,7,0xe0,7,0,0,0,0}};
    IDirect3DTexture9* alpha=nullptr;
    Check(CreateReplacement(dev,rgba,&alpha),"factory uploads DXT5 alpha material");
    Check(alpha&&ReadPixel(dev,alpha,0x4000ff00),"DXT5 decoded RGB/alpha readback preserved");if(alpha)alpha->Release();
    dev->CreateTexture(4,4,1,0,D3DFMT_DXT1,ex?D3DPOOL_DEFAULT:D3DPOOL_MANAGED,&source,nullptr);
    Check(source&&texpositive::Store(source,replacement),"source owns replacement metadata without source pin");
    unsigned before=calls;
    hr=SingularityTextureBridgeV1(dev,1,source,Forward,1,0);
    IDirect3DBaseTexture9* bound=nullptr;dev->GetTexture(1,&bound);
    Check(hr==E_ACCESSDENIED&&calls==before+1&&bound==replacement,"positive callback forwards replacement once, preserves stage/HRESULT");if(bound)bound->Release();dev->SetTexture(1,nullptr);
    before=calls;hr=SingularityTextureBridgeV1(dev,1,source,Forward,1,texbridge::protectedSource);dev->GetTexture(1,&bound);
    Check(hr==E_ACCESSDENIED&&calls==before+1&&bound==source,"protected menu/shadow source bypassed");if(bound)bound->Release();dev->SetTexture(1,nullptr);
    before=calls;hr=SingularityTextureBridgeV1(dev,1,source,Forward,99,0);
    Check(hr==E_ACCESSDENIED&&calls==before+1,"unknown ABI fails closed to original");dev->SetTexture(1,nullptr);
    hr=ex?devEx->ResetEx(&pp,nullptr):dev->Reset(&pp);
    Check(SUCCEEDED(hr),"native Reset / ResetEx succeeds with held replacement");
    auto* cached=texpositive::Acquire(source);Check(cached==replacement,"resource metadata retained across reset");if(cached)cached->Release();
    Check(ReadPixel(dev,replacement,0xff0000),"replacement pixels preserved after reset");
    texbridge::Invalidate(source);cached=texpositive::Acquire(source);Check(!cached,"write invalidation drops positive cache");if(cached)cached->Release();
    Check(texpositive::Store(source,replacement),"metadata restored for destruction check");
    Check(source->Release()==0,"cache does not pin original lifetime");
    Check(replacement->Release()==0,"source destruction releases replacement ownership");
    // Exercise the actual Seeker lifecycle with bounded synthetic full chains.
    seekerpreload::mode=2;seekerpreload::loaded=true;seekerpreload::thread=GetCurrentThreadId();
    for(unsigned k=0;k<2;++k){auto& c=seekerpreload::cpu[k];c={};c.w=4096;c.h=2048;c.levels=13;
        c.fmt=k?D3DFMT_DXT1:D3DFMT_DXT5;
        for(unsigned level=0;level<13;++level){size_t bytes=size_t((((std::max)(1u,c.w>>level))+3)/4)*((((std::max)(1u,c.h>>level))+3)/4)*(k?8:16);
            c.mip.emplace_back(bytes);for(size_t n=0;n<bytes;n+=k?8:16){auto* block=c.mip.back().data()+n;
                if(!k){block[0]=block[1]=255;block+=8;}block[0]=block[2]=0;block[1]=block[3]=0xf8;}}
        Check(seekerpreload::Validate(c,k),"Seeker bounded full mip chain");}
    Check(!seekerpreload::Pinned("this-private-fixture-does-not-exist.dds",seekerSha[0]),"missing preload file refused");
    const auto normalFormat=seekerpreload::cpu[1].fmt;
    seekerpreload::cpu[1].fmt=D3DFMT_UNKNOWN;
    Check(!seekerpreload::Validate(seekerpreload::cpu[1],1),"incompatible normal encoding refused");
    seekerpreload::Lifecycle(dev,2);
    Check(!seekerpreload::gpu[0]&&!seekerpreload::gpu[1],"second upload failure publishes neither resource");
    seekerpreload::cpu[1].fmt=normalFormat;
    seekerpreload::Lifecycle(dev,2);
    Check(seekerpreload::gpu[0]&&seekerpreload::gpu[1],"Seeker pair atomically uploaded outside bind");
    IDirect3DTexture9* marker=nullptr;dev->CreateTexture(4,4,1,0,D3DFMT_DXT1,ex?D3DPOOL_DEFAULT:D3DPOOL_MANAGED,&marker,nullptr);
    const unsigned reads=seekerpreload::reads,uploads=seekerpreload::uploads;
    for(unsigned k=0;k<2;++k){marker->SetPrivateData(texbridge::seekerId,&k,sizeof(k),0);
        for(unsigned eye=0;eye<2;++eye){SingularityTextureBridgeV1(dev,eye,marker,Forward,1,0);
            IDirect3DBaseTexture9* current=nullptr;dev->GetTexture(eye,&current);
            Check(current==seekerpreload::gpu[k],"Seeker both-eye stages use prepared resource");if(current)current->Release();dev->SetTexture(eye,nullptr);}
        Check(ReadPixel(dev,seekerpreload::gpu[k],0xff0000),"Seeker native uploaded pixel");}
    for(unsigned mode=0;mode<2;++mode){seekerpreload::mode=int(mode);
        Check(seekerpreload::Ready(dev,0,marker)==marker,"Seeker OFF/log-only original forwarding");}
    seekerpreload::mode=2;
    for(unsigned n=0;n<10000;++n)SingularityTextureBridgeV1(dev,0,marker,Forward,1,0);
    dev->SetTexture(0,nullptr);
    Check(reads==seekerpreload::reads&&uploads==seekerpreload::uploads,"10000 Seeker binds: zero file reads or uploads");
    texbridge::Invalidate(marker);unsigned index=0;DWORD size=sizeof(index);
    Check(FAILED(marker->GetPrivateData(texbridge::seekerId,&index,&size)),"Seeker source write invalidates scalar identity");
    marker->Release();seekerpreload::Lifecycle(dev,1);
    Check(!seekerpreload::gpu[0]&&!seekerpreload::owner,"Seeker pre-reset releases pair");
    Check(SUCCEEDED(ex?devEx->ResetEx(&pp,nullptr):dev->Reset(&pp)),"Seeker Reset/ResetEx");
    seekerpreload::Lifecycle(dev,2);Check(seekerpreload::gpu[0]&&ReadPixel(dev,seekerpreload::gpu[0],0xff0000),"Seeker recreate from immutable CPU cache");
    seekerpreload::Lifecycle(dev,1);seekerpreload::loaded=false;seekerpreload::Lifecycle(dev,2);
    Check(!seekerpreload::gpu[0],"failed preload never uploads or delays into bind");
    seekerpreload::mode=0;seekerpreload::cpu[0]={};seekerpreload::cpu[1]={};
    Check(dev->Release()==0,"device destruction has no Seeker COM leak");DestroyWindow(window);
}
int main(){
    Check(remasterscope::Contains("AR_shotgun_d")&&remasterscope::Contains("inview_glove_n"),"exact remaster material scope");
    Check(!remasterscope::Contains("360_A")&&!remasterscope::Contains("brightFlash_01")&&!remasterscope::Contains("AR_shotgun_d_more")&&!remasterscope::Contains(nullptr),"glyphs/FX/substring/unknown are not remastered");
    Check(remasterscope::Contains("brightFlash_01",true)&&remasterscope::Contains("bsplat_decal_d",true)&&
        !remasterscope::Contains("bsplat_decal_d")&&!remasterscope::Contains("360_A",true)&&
        !remasterscope::Contains("brightFlash_01_more",true),"effects require exact explicit opt-in; glyphs/substring remain protected");
    HMODULE system=LoadLibraryExW(L"d3d9.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
    auto plain=reinterpret_cast<IDirect3D9* (WINAPI*)(UINT)>(GetProcAddress(system,"Direct3DCreate9"));
    auto createEx=reinterpret_cast<HRESULT (WINAPI*)(UINT,IDirect3D9Ex**)>(GetProcAddress(system,"Direct3DCreate9Ex"));
    IDirect3D9* api=plain(D3D_SDK_VERSION);IDirect3D9Ex* ex=nullptr;Check(SUCCEEDED(createEx(D3D_SDK_VERSION,&ex))&&ex,"native Ex interface");
    Run(api,nullptr,false);if(ex)Run(nullptr,ex,true);if(ex)ex->Release();api->Release();FreeLibrary(system);return failures?1:0;
}
