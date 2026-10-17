#pragma once
#include "projector_receiver_math.h"
namespace projectorfix {
static bool enabled=false,fault=false,vsMatch=false,psMatch=false,nativeValid=false;
static volatile LONG configured=0;
static thread_local unsigned pairDepth=0;
struct PairScope{bool active;PairScope():active(enabled){if(active)++pairDepth;}~PairScope(){if(active)--pairDepth;}};
static float nativeVP[16]{},trackedBase[16]{};
struct Entry{IUnknown* shader;bool match;};
static Entry cache[128]{};static unsigned used=0,nextEviction=0;
static unsigned classified=0,evictions=0,vsHits=0,psHits=0,pairHits=0,routeRejects=0,shapeRejects=0,cameraRejects=0;
static unsigned applied[2]{},rejected=0,restoreFailures=0;
inline void Release(){for(unsigned i=0;i<used;++i)cache[i].shader->Release();used=nextEviction=0;vsMatch=psMatch=nativeValid=false;}
inline void Configure(bool on){InterlockedExchange(&configured,on?1:0);}
template<class T> bool Classify(T* s,UINT expected,unsigned long long hash){
    if(!enabled||fault||!s)return false;
    for(unsigned i=0;i<used;++i)if(cache[i].shader==s)return cache[i].match;
    ++classified;
    DWORD code[334]{};UINT bytes=0;bool match=false;
    if(SUCCEEDED(s->GetFunction(nullptr,&bytes))&&bytes==expected){UINT capacity=sizeof(code);
        if(SUCCEEDED(s->GetFunction(code,&capacity))&&capacity==bytes){
            unsigned long long h=14695981039346656037ull;
            for(UINT i=0;i<bytes;++i){h^=((unsigned char*)code)[i];h*=1099511628211ull;}
            match=h==hash;}}
    // Bound retained COM references, not the set of shaders we can recognize.
    // The projector can first appear well after the initial 128 menu/world shaders.
    unsigned slot=used;
    if(used==128){slot=nextEviction;nextEviction=(nextEviction+1)%128;cache[slot].shader->Release();++evictions;}
    else ++used;
    s->AddRef();cache[slot]={s,match};return match;
}
inline void BindVS(IDirect3DVertexShader9* s){vsMatch=Classify(s,208,0xAC4827843287AECBull);if(vsMatch)++vsHits;}
inline void BindPS(IDirect3DPixelShader9* s){psMatch=Classify(s,1336,0x7F00BC7925591BE8ull);if(psMatch)++psHits;}
inline void NoteCamera(UINT reg,int conv,const float* original,const float* modified){
    if(!enabled||reg!=0)return;
    nativeValid=conv==0&&shadowreceiver::Finite(original)&&shadowreceiver::Finite(modified);
    if(nativeValid){memcpy(nativeVP,original,sizeof(nativeVP));memcpy(trackedBase,modified,sizeof(trackedBase));}
}
inline void Poll(){
    static DWORD last=0;const DWORD now=GetTickCount();if(now-last<1000)return;last=now;
    const bool on=InterlockedCompareExchange(&configured,0,0)!=0;
    if(on!=enabled){Release();enabled=on;Log("PROJECTOR_RECONSTRUCTION enabled=%d",on?1:0);}
    if(enabled&&(classified||vsHits||psHits||pairHits||rejected||restoreFailures)){
        Log("PROJECTOR_RECONSTRUCTION left=%u right=%u rejected=%u restoreFailures=%u fault=%d",applied[0],applied[1],rejected,restoreFailures,fault?1:0);
        Log("PROJECTOR_GATES classified=%u evictions=%u vs=%u ps=%u pair=%u routeRejected=%u shapeRejected=%u cameraRejected=%u",classified,evictions,vsHits,psHits,pairHits,routeRejects,shapeRejects,cameraRejects);
        applied[0]=applied[1]=rejected=restoreFailures=0;}
    classified=evictions=vsHits=psHits=pairHits=routeRejects=shapeRejects=cameraRejects=0;
}
struct Scope {
    IDirect3DDevice9* dev;float saved[16]{};bool changed=false;
    Scope(IDirect3DDevice9* d,bool eligible,UINT verts,UINT prims):dev(d){
        if(!enabled||fault||!vsMatch||!psMatch)return;
        ++pairHits;
        if(!eligible){++routeRejects;return;}
        if(verts!=4||prims!=2){++shapeRejects;return;}
        bool camera=false;
        for(int i=0;i<g_camMatUsed;++i)if(g_camMats[i].valid&&g_camMats[i].reg==0&&g_camMats[i].conv==0&&
            nativeValid&&!memcmp(g_camMats[i].m,trackedBase,sizeof(trackedBase)))camera=true;
        if(!camera){++rejected;++cameraRejects;return;}
        IDirect3DBaseTexture9* textures[2]{};IDirect3DSurface9* target=nullptr;
        D3DSURFACE_DESC rt{},a{},b{};RECT sc{};D3DVIEWPORT9 vp{};DWORD scOn=0;
        bool resources=SUCCEEDED(dev->GetRenderTarget(0,&target))&&target&&SUCCEEDED(target->GetDesc(&rt));
        for(unsigned i=0;i<2;++i)resources=SUCCEEDED(dev->GetTexture(i,&textures[i]))&&textures[i]&&resources;
        resources=resources&&textures[0]->GetType()==D3DRTYPE_TEXTURE&&textures[1]->GetType()==D3DRTYPE_TEXTURE&&
            SUCCEEDED(((IDirect3DTexture9*)textures[0])->GetLevelDesc(0,&a))&&
            SUCCEEDED(((IDirect3DTexture9*)textures[1])->GetLevelDesc(0,&b))&&
            SUCCEEDED(dev->GetScissorRect(&sc))&&SUCCEEDED(dev->GetViewport(&vp))&&
            SUCCEEDED(dev->GetRenderState(D3DRS_SCISSORTESTENABLE,&scOn));
        for(auto t:textures)if(t)t->Release();if(target)target->Release();
        const bool right=sc.left==(LONG)rt.Width/2&&sc.right==(LONG)rt.Width;
        const bool left=sc.left==0&&sc.right==(LONG)rt.Width/2;
        resources=resources&&rt.Width>1&&!(rt.Width%2)&&rt.Format==D3DFMT_A8R8G8B8&&
            a.Width==rt.Width&&a.Height==rt.Height&&a.Format==D3DFMT_A16B16G16R16F&&
            b.Width==2048&&b.Height==2048&&b.Format==D3DFMT_DXT1&&scOn&&
            (left||right)&&sc.top==0&&sc.bottom==(LONG)rt.Height&&
            vp.X==0&&vp.Y==0&&vp.Width==rt.Width&&vp.Height==rt.Height;
        float depth[4]{},eye[16]{},corrected[16]{};
        if(!resources||FAILED(dev->GetPixelShaderConstantF(2,depth,1))||!worldshadowmath::WorldDepth(depth)||
            FAILED(dev->GetVertexShaderConstantF(0,eye,4))||FAILED(dev->GetPixelShaderConstantF(3,saved,4))||
            !projectormath::Correct(nativeVP,eye,saved,corrected)){++rejected;return;}
        changed=true;
        if(FAILED(dev->SetPixelShaderConstantF(3,corrected,4))){
            ++rejected;if(FAILED(dev->SetPixelShaderConstantF(3,saved,4))){++restoreFailures;fault=true;}
            changed=false;return;}
        ++applied[right?1:0];
    }
    Scope(const Scope&)=delete;Scope& operator=(const Scope&)=delete;
    ~Scope(){if(changed&&FAILED(dev->SetPixelShaderConstantF(3,saved,4))){++restoreFailures;fault=true;}}
};
}
