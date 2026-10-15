#pragma once
#include "native_muzzle_policy.h"
// Included after D3D9/Log/skeletal declarations. No new hooks. Classify only
// at existing shader setters, keep bounded references until Reset/shutdown.
namespace nativefx {
static Selection selected;
static bool fault=false,context=false;
static volatile LONG probe=0;
inline bool Probe(){return InterlockedCompareExchange(&probe,0,0)!=0;}
static volatile LONG shots=0;
struct Shader {IUnknown* object=nullptr;std::uint64_t hash=0;unsigned bytes=0;int camera=-1;};
static Shader shaders[1024],vs,ps;static unsigned used=0,overflow=0;
static unsigned dropped=0,aliases=0,refused=0,restoreFailed=0;
static CapturePlan capture;
static unsigned captureWeapon=0;
static ULONGLONG probeStart=0;
struct Witness {std::uint64_t vs=0,ps=0;unsigned vb=0,pb=0,slots=0;int camera=-1,alias=-1;
    bool read=false,tracked=false;float gpu[16]{};};
static Witness witness[128];static unsigned witnessCount=0,witnessOverflow=0;
template<class Dev,class Slot> void Observe(Dev* dev,const Slot* slots,int n,bool allowed){
    if(!Probe()||!shadowtrace::active||!allowed)return;
    for(unsigned i=0;i<witnessCount;++i)if(witness[i].vs==vs.hash&&witness[i].ps==ps.hash)return;
    if(witnessCount==128){++witnessOverflow;return;}
    auto& w=witness[witnessCount++];w={};w.vs=vs.hash;w.ps=ps.hash;w.vb=vs.bytes;w.pb=ps.bytes;w.camera=vs.camera;
    for(int i=0;i<n&&i<32;++i)if(slots[i].valid){w.slots|=1u<<i;if((int)slots[i].reg==vs.camera)w.tracked=true;}
    if(vs.camera>=0&&SUCCEEDED(dev->GetVertexShaderConstantF((UINT)vs.camera,w.gpu,4))){
        w.read=true;w.alias=Alias(w.gpu,(unsigned)vs.camera,slots,n);
    }
}
inline void DumpWitness(){
    if(!Probe())return;
    Log("native-fx: camera witness records=%u overflow=%u (StereoPair entry only; raw draws still in pass trace)",witnessCount,witnessOverflow);
    for(unsigned i=0;i<witnessCount;++i){const auto& w=witness[i];
        Log("native-fx witness VS=%016llX VSBytes=%u PS=%016llX PSBytes=%u camera=%d tracked=%d validSlots=%08X read=%d alias=%d",
            w.vs,w.vb,w.ps,w.pb,w.camera,w.tracked?1:0,w.slots,w.read?1:0,w.alias);
        if(w.read)for(unsigned r=0;r<4;++r)Log("native-fx matrix VS=%016llX PS=%016llX row=%u value=%.9g,%.9g,%.9g,%.9g",
            w.vs,w.ps,r,w.gpu[r*4],w.gpu[r*4+1],w.gpu[r*4+2],w.gpu[r*4+3]);
    }
    witnessCount=witnessOverflow=0;
    if(capture.stage==4){InterlockedExchange(&probe,0);Log("native-fx: automatic capture complete; probe overhead disabled, not visual acceptance");}
}
inline bool Enabled(){return Probe()||selected.Valid();}
template<class T> Shader Describe(T* shader,bool vertex){
    if(!Enabled()||!shader)return {};
    unsigned slot=1024;const unsigned first=(unsigned)(((uintptr_t)shader>>4)*2654435761u)&1023;
    for(unsigned i=0;i<1024;++i){const unsigned s=(first+i)&1023;
        if(shaders[s].object==shader)return shaders[s];
        if(!shaders[s].object){slot=s;break;}}
    if(slot==1024){++overflow;return {};}
    // No draw-time bytecode calls, heap work or disk output. Truncation is unknown.
    static thread_local DWORD code[8192];UINT bytes=sizeof(code);Shader result;
    if(SUCCEEDED(shader->GetFunction(code,&bytes))&&bytes>=8&&bytes%4==0&&bytes<=sizeof(code)){
        result.hash=Hash(code,bytes);result.bytes=bytes;
        if(vertex)result.camera=skeletal::ReadLayout(reinterpret_cast<const uint32_t*>(code),bytes/4).viewProjection;
    }
    shader->AddRef();result.object=shader;shaders[slot]=result;++used;return result;
}
inline void BindVS(IDirect3DVertexShader9* shader){vs=Describe(shader,true);}
inline void BindPS(IDirect3DPixelShader9* shader){ps=Describe(shader,false);}
inline bool Match(){return !fault&&context&&selected.Match(vs.hash,ps.hash,vs.bytes,ps.bytes);}
inline bool Drop(){if(selected.mode==1&&Match()){++dropped;return true;}return false;}
inline void Release(){
    for(auto& shader:shaders){if(shader.object)shader.object->Release();shader={};}
    used=0;vs={};ps={};context=false;witnessCount=witnessOverflow=0;
}
inline void Configure(int mode,bool trace,const char* v,const char* p,unsigned vb,unsigned pb){
    Release();selected={};InterlockedExchange(&probe,trace?1:0);fault=false;capture={};captureWeapon=0;probeStart=0;InterlockedExchange(&shots,0);
    if(mode==1||mode==2){Selection x;x.mode=mode;x.vsBytes=vb;x.psBytes=pb;
        if(Hex(v,x.vs)&&Hex(p,x.ps)&&x.Valid())selected=x;}
    Log("native-fx: probe=%d mode=%d (0 unchanged,1 selected-pair drop,2 exact camera alias); no replacement FX",Probe()?1:0,selected.mode);
}
// Snapshot before StereoPair. The old cache + GPU block are restored even if
// the stereo draw returns a failed HRESULT. Never borrow stale/unverified data.
template<class Dev,class Slot,class Upload> struct AliasScope {
    Dev* dev;Slot* slots;int& usedSlots;Upload upload;int originalUsed,index=-1;
    Slot saved{};float gpu[16]{};unsigned reg=0;bool changed=false;
    AliasScope(Dev* d,Slot* s,int& n,int cap,Upload u,bool allowed):
        dev(d),slots(s),usedSlots(n),upload(u),originalUsed(n){
        if(!allowed||selected.mode!=2||!Match()||vs.camera<0||!u)return;
        reg=(unsigned)vs.camera;
        if(reg>252||FAILED(d->GetVertexShaderConstantF(reg,gpu,4))){++refused;return;}
        const int source=Alias(gpu,reg,s,n);if(source<0){++refused;return;}
        for(int i=0;i<n;++i)if(!s[i].valid){index=i;break;}
        if(index<0&&n<cap)index=n++;
        if(index<0){++refused;return;}
        saved=s[index];s[index]=s[source];s[index].reg=reg;changed=true;++aliases;
    }
    ~AliasScope(){if(changed){
        slots[index]=saved;usedSlots=originalUsed;
        if(FAILED(upload(dev,reg,gpu,4))){fault=true;++restoreFailed;}
    }}
};
inline void Boundary(bool ready,bool busy,unsigned weapon){
    context=ready;
    if(!Probe())return;
    if(ready&&!probeStart)probeStart=GetTickCount64();
    if(probeStart&&GetTickCount64()-probeStart>120000){
        InterlockedExchange(&probe,0);Log("native-fx: two-minute diagnostic timeout; incomplete capture, not a PASS");return;
    }
    if(!busy&&shadowtrace::sequence>=4){InterlockedExchange(&probe,0);Log("native-fx: shared trace budget unavailable; no further captures");return;}
    if(!capture.stage)captureWeapon=weapon;
    if(ready&&capture.stage&&capture.stage<4&&weapon!=captureWeapon){
        capture.stage=4;InterlockedExchange(&probe,0);Log("native-fx: capture ABORTED weapon changed; do not compare different weapons");return;
    }
    const int next=capture.Tick(ready,InterlockedCompareExchange(&shots,0,0),busy);
    if(next&&!shadowtrace::pending&&!shadowtrace::active&&shadowtrace::sequence<4){
        shadowtrace::pending=true;
        Log("native-fx: capture=%u label=%s shotSerial=%ld; next FULL frame, not guaranteed first flash frame",
            capture.stage,next==1?"idle":"post-shot",InterlockedCompareExchange(&shots,0,0));
    }
}
inline void Summary(){
    if(!Enabled())return;
    static ULONGLONG last=0;const ULONGLONG now=GetTickCount64();if(now-last<2000)return;last=now;
    if(dropped||aliases||refused||restoreFailed||overflow){
        Log("native-fx: selected drop=%u alias=%u refused=%u restoreFailed=%u shaderOverflow=%u fault=%d",dropped,aliases,refused,restoreFailed,overflow,fault?1:0);
        dropped=aliases=refused=restoreFailed=overflow=0;
    }
}
} // namespace nativefx
