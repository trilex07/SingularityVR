#include <windows.h>
#include <unknwn.h>
#include <cstring>
#include <cassert>
#include "../shadow_world_receiver_math.h"
static void Log(const char*,...){}
enum {D3DRTYPE_TEXTURE=3,D3DFMT_A8R8G8B8=21,D3DFMT_A16B16G16R16F=113,D3DFMT_DXT1=827611204,D3DRS_SCISSORTESTENABLE=174};
struct D3DSURFACE_DESC{UINT Width=5904,Height=3012;int Format=21;};
struct D3DVIEWPORT9{DWORD X=0,Y=0,Width=5904,Height=3012;float MinZ=0,MaxZ=1;};
struct IDirect3DVertexShader9:IUnknown{HRESULT GetFunction(void*,UINT*){assert(false);return E_FAIL;}};
struct IDirect3DPixelShader9:IUnknown{HRESULT GetFunction(void*,UINT*){assert(false);return E_FAIL;}};
struct IDirect3DBaseTexture9{unsigned refs=1;int GetType(){return D3DRTYPE_TEXTURE;}void Release(){assert(refs>1);--refs;}};
struct IDirect3DTexture9:IDirect3DBaseTexture9{D3DSURFACE_DESC desc;HRESULT GetLevelDesc(UINT,D3DSURFACE_DESC* p){*p=desc;return S_OK;}};
struct IDirect3DSurface9{unsigned refs=1;D3DSURFACE_DESC desc;void Release(){assert(refs>1);--refs;}HRESULT GetDesc(D3DSURFACE_DESC* p){*p=desc;return S_OK;}};
struct CamMatSlot{UINT reg=0;int conv=0;bool valid=true;float m[16]={1,0,0,0,0,1,0,0,0,0,1,1,0,0,-10,0};};
static CamMatSlot g_camMats[1];static int g_camMatUsed=1;
struct IDirect3DDevice9{
    IDirect3DSurface9 target;IDirect3DTexture9 textures[2];unsigned reads=0,writes=0;
    bool failWrite=false,failRestore=false;float ps[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    IDirect3DDevice9(){textures[0].desc.Format=113;textures[1].desc={2048,2048,D3DFMT_DXT1};}
    HRESULT GetRenderTarget(UINT,IDirect3DSurface9** p){++reads;++target.refs;*p=&target;return S_OK;}
    HRESULT GetTexture(UINT i,IDirect3DBaseTexture9** p){++textures[i].refs;*p=&textures[i];return S_OK;}
    HRESULT GetScissorRect(RECT* p){*p={0,0,2952,3012};return S_OK;}
    HRESULT GetViewport(D3DVIEWPORT9* p){*p={};return S_OK;}
    HRESULT GetRenderState(int,DWORD* p){*p=1;return S_OK;}
    HRESULT GetPixelShaderConstantF(UINT r,float* p,UINT n){if(r==2){assert(n==1);const float d[]={9.99f,.001f,1/9.99f,.001f/9.99f};memcpy(p,d,16);}else{assert(r==3&&n==4);memcpy(p,ps,64);}return S_OK;}
    HRESULT GetVertexShaderConstantF(UINT r,float* p,UINT n){assert(r==0&&n==4);memcpy(p,g_camMats[0].m,64);p[0]*=.5f;p[8]=-.5f;return S_OK;}
    HRESULT SetPixelShaderConstantF(UINT r,const float* p,UINT n){assert(r==3&&n==4);memcpy(ps,p,64);++writes;return (writes==1&&failWrite)||(writes==2&&failRestore)?E_FAIL:S_OK;}
};
#include "../projector_receiver_fix.h"
struct CacheShader:IUnknown {
    ULONG refs=1;DWORD code=0;unsigned queries=0;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID,void**) override{return E_NOINTERFACE;}
    ULONG STDMETHODCALLTYPE AddRef() override{return ++refs;}
    ULONG STDMETHODCALLTYPE Release() override{assert(refs>1);return --refs;}
    HRESULT GetFunction(void* data,UINT* bytes){++queries;if(data){assert(*bytes>=4);memcpy(data,&code,4);}*bytes=4;return S_OK;}
};
int main(){using namespace projectorfix;IDirect3DDevice9 d;float original[16];memcpy(original,d.ps,64);
    // A late-loaded target must remain recognizable after the bounded cache fills.
    CacheShader shaders[129];shaders[128].code=1;
    unsigned long long hash=14695981039346656037ull;
    for(unsigned i=0;i<4;++i){hash^=((unsigned char*)&shaders[128].code)[i];hash*=1099511628211ull;}
    assert(!Classify(&shaders[128],4,hash)&&!shaders[128].queries);
    enabled=true;
    for(unsigned i=0;i<128;++i)assert(!Classify(&shaders[i],4,hash));
    assert(Classify(&shaders[128],4,hash)&&used==129&&evictions==0);
    const auto queries=shaders[128].queries;
    assert(Classify(&shaders[128],4,hash)&&shaders[128].queries==queries);
    assert(shaders[0].refs==2&&shaders[128].refs==2);
    for(unsigned pass=0;pass<100;++pass)for(unsigned i=0;i<129;++i)
        assert(Classify(&shaders[i],4,hash)==(i==128));
    for(auto& shader:shaders)assert(shader.queries==2);
    Release();for(auto& shader:shaders)assert(shader.refs==1);
    assert(used==0);
    // Overflow every set: bounded ownership, late target still recognized.
    static CacheShader overflow[cacheSets*cacheWays+1];
    for(auto& shader:overflow)Classify(&shader,4,hash);
    assert(used<=cacheSets*cacheWays&&evictions>0);
    overflow[cacheSets*cacheWays].code=1;
    Release();
    assert(Classify(&overflow[cacheSets*cacheWays],4,hash));
    Release();for(auto& shader:overflow)assert(shader.refs==1);
    enabled=false;
    {Scope s(&d,true,4,2);}assert(!d.reads&&!d.writes);
    enabled=vsMatch=psMatch=true;NoteCamera(0,0,g_camMats[0].m,g_camMats[0].m);
    {Scope s(&d,false,4,2);}{Scope s(&d,true,8,12);}assert(!d.reads);
    g_camMats[0].valid=false;{Scope s(&d,true,4,2);}assert(!d.reads);g_camMats[0].valid=true;
    {Scope s(&d,true,4,2);assert(s.changed&&memcmp(original,d.ps,64));}
    assert(d.writes==2&&!memcmp(original,d.ps,64));assert(d.target.refs==1&&d.textures[0].refs==1&&d.textures[1].refs==1);
    d.writes=0;d.failWrite=true;{Scope s(&d,true,4,2);assert(!s.changed);}assert(d.writes==2&&!memcmp(original,d.ps,64));
    d.writes=0;d.failWrite=false;d.failRestore=true;{Scope s(&d,true,4,2);}assert(fault&&restoreFailures==1);
    const auto writes=d.writes;{Scope s(&d,true,4,2);}assert(d.writes==writes);
}
