#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d9.h>
#include <cassert>
#include <cstdio>
#include <cstdarg>
#include "../skeletal_palette.h"
static void Log(const char*,...){}
namespace shadowtrace {static bool pending=false,active=false;static unsigned sequence=0;}
#include "../native_muzzle_stereo.h"
struct Slot {unsigned reg=0;int conv=0;bool valid=false;float m[16]{};};
struct Dev {float matrix[16]{};unsigned reads=0,writes=0;bool failRead=false,failWrite=false;
    HRESULT GetVertexShaderConstantF(UINT,float* p,UINT){++reads;if(failRead)return E_FAIL;memcpy(p,matrix,64);return S_OK;}
};
static HRESULT Upload(Dev* d,UINT,const float* p,UINT){++d->writes;memcpy(d->matrix,p,64);return d->failWrite?E_FAIL:S_OK;}
using Scope=nativefx::AliasScope<Dev,Slot,decltype(&Upload)>;
struct FakeShader: IUnknown {
    ULONG refs=1;unsigned queries=0;bool fail=false;DWORD words[22]{};
    FakeShader(){words[0]=0xfffe0300;words[1]=(19<<16)|0xfffe;words[2]=0x42415443;
        words[3]=28;words[6]=1;words[7]=28;words[10]=48;words[11]=2|(12<<16);words[12]=4;
        memcpy(words+15,"ViewProjectionMatrix",21);words[21]=0xffff;}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID,void**)override{return E_NOINTERFACE;}
    ULONG STDMETHODCALLTYPE AddRef()override{return ++refs;}
    ULONG STDMETHODCALLTYPE Release()override{assert(refs>1);return --refs;}
    HRESULT GetFunction(void* p,UINT* n){++queries;if(fail||*n<sizeof(words))return E_FAIL;*n=sizeof(words);memcpy(p,words,sizeof(words));return S_OK;}
};
static void Init(Dev& d,Slot* slots,int& n){
    using namespace nativefx;selected={11,22,128,256,2,1};fault=false;context=true;vs={};ps={};
    vs.hash=11;vs.bytes=128;vs.camera=12;ps.hash=22;ps.bytes=256;
    n=1;slots[0]={};slots[0].reg=0;slots[0].valid=true;slots[0].conv=1;
    for(unsigned i=0;i<4;++i)d.matrix[i*4+i]=slots[0].m[i*4+i]=1;
}
int main(){
    using namespace nativefx;
    {const Selection family[]={
        {0x314D478A9CFDBE37ull,0xC118540CCDBCB241ull,3384,448,4},
        {0x488BAEB579E71A73ull,0x27DFE7D9C67BCC03ull,2968,452,4},
        {0x4FAEF2389750DBFCull,0x2FEC493CD52D7D78ull,2908,336,4}};
     Slot camera[2]{};camera[0].valid=true;camera[0].reg=0;camera[0].conv=0;
     for(unsigned i=0;i<4;++i)camera[0].m[5*i]=1;
     assert(ParticleCameraReady(camera,1));
     camera[0].conv=1;assert(!ParticleCameraReady(camera,1));camera[0].conv=0;
     camera[0].m[0]=NAN;assert(!ParticleCameraReady(camera,1));camera[0].m[0]=1;
     camera[1]=camera[0];assert(!ParticleCameraReady(camera,2));
     camera[1].reg=5;assert(!ParticleCameraReady(camera,2));
     camera[1].reg=9;assert(ParticleCameraReady(camera,2));
     camera[0].valid=false;assert(!ParticleCameraReady(camera,2));camera[0].valid=true;
     for(int i=0;i<3;++i){
         const auto& f=family[i];Configure(1,false,"4FAEF2389750DBFC","2FEC493CD52D7D78",2908,336,-1);
         context=true;vs.hash=f.vs;vs.bytes=f.vsBytes;vs.camera=0;vs.local=5;ps.hash=f.ps;ps.bytes=f.psBytes;
         assert(ParticleTarget()<0);ConfigureParticles(true);assert(ParticleTarget()==i);
         ++ps.bytes;assert(ParticleTarget()<0);--ps.bytes;
         ps.hash^=1;assert(ParticleTarget()<0);ps.hash^=1;
         vs.camera=12;assert(ParticleTarget()<0);vs.camera=0;
         vs.local=6;assert(ParticleTarget()<0);vs.local=5;
         context=false;assert(ParticleTarget()<0);context=true;
         {ParticleScope denied(i,false);assert(!denied.active&&!particleDepth);}
         assert(particleStats[i].refused==1);
         assert(Drop()==(i==2));
         {ParticleScope scope(ParticleTarget(),ParticleCameraReady(camera,1));
             assert(scope.active&&particleDepth==1&&!Drop());
             {ParticleScope inner(i,true);assert(particleDepth==2&&!Drop());}
             assert(particleDepth==1);
         }
         assert(!particleDepth&&Drop()==(i==2));
     }
     Dev d;ConfigureParticles(true);
     for(unsigned n=0;n<6;++n){ParticleLocalWitness<Dev> w(&d,0);}
     assert(d.reads==6&&d.writes==0&&particleStats[0].restored==3);
     {ParticleLocalWitness<Dev> w(&d,1);d.matrix[0]=1;}
     assert(!particleStereo&&particleStats[1].restoreMismatch==1);
     assert(Drop()); // failed witness disables only new route, accepted drop survives
     Configure(0,false,"","",0,0,-1);assert(!particleStereo&&!particleDepth);
    }
    {std::uint32_t words[22]{};words[0]=0xfffe0300;words[1]=(19<<16)|0xfffe;words[2]=0x42415443;
        words[3]=28;words[6]=1;words[7]=28;words[10]=48;words[11]=2|(12<<16);words[12]=4;
        memcpy(words+15,"ViewProjectionMatrix",21);words[21]=0xffff;
        assert(skeletal::ReadLayout(words,22).viewProjection==12);
        words[12]=3;assert(skeletal::ReadLayout(words,22).viewProjection<0);
        words[12]=4;words[11]=2|(253<<16);assert(skeletal::ReadLayout(words,22).viewProjection<0);
    }
    std::uint64_t h=7;assert(!Hex("0",h)&&h==7);assert(!Hex("0000000000000000",h));
    assert(!Hex("gggggggggggggggg",h));assert(Hex("CBF29CE484222325",h));assert(h==Hash("",0));
    assert(Hash("hello",5)==0xa430d84680aabd0bull);
    {Selection unknown{11,22,128,256,2};assert(!unknown.Valid());unknown.convention=0;assert(unknown.Valid());}
    {Selection observe{11,22,128,256,3};assert(observe.Valid());
     selected=observe;context=true;fault=false;vs.hash=11;vs.bytes=128;ps.hash=22;ps.bytes=256;
     assert(Match()&&!Drop());selected={};vs={};ps={};}
    {Selection world{11,22,128,256,4};assert(world.Valid());
     selected=world;context=true;fault=false;vs.hash=11;vs.bytes=128;ps.hash=22;ps.bytes=256;
     assert(WorldRoute()&&!Drop());WorldCameraObserve(true,true);
     assert(worldRerouted==1&&worldCameraValid==1&&worldCameraC0==1);
     context=false;assert(!WorldRoute());WorldCameraObserve(false,false);
     assert(worldCameraFallback==0);
     context=true;ps.hash=23;assert(!WorldRoute());selected={};vs={};ps={};}
    {Configure(1,false,"4FAEF2389750DBFC","2FEC493CD52D7D78",2908,336,-1,true);
     ConfigureWorldRoute(true,"488BAEB579E71A73","27DFE7D9C67BCC03",2968,452);
     assert(selected.mode==1&&worldSelected.Valid());context=true;
     vs.hash=selected.vs;vs.bytes=selected.vsBytes;ps.hash=selected.ps;ps.bytes=selected.psBytes;
     assert(Drop()&&!WorldRoute());
     vs.hash=worldSelected.vs;vs.bytes=worldSelected.vsBytes;
     ps.hash=worldSelected.ps;ps.bytes=worldSelected.psBytes;
     assert(!Drop()&&WorldRoute());WorldCameraObserve(true,true);
     assert(worldRerouted==1&&worldCameraValid==1&&worldCameraC0==1);
     const LONG observed=routeCounts[10];RouteObserve(10);assert(routeCounts[10]==observed+1);
     ConfigureWorldRoute(true,"4FAEF2389750DBFC","2FEC493CD52D7D78",2908,336);
     assert(!worldSelected.Valid()&&!WorldRoute());
     Configure(0,false,"","",0,0,-1);assert(!selected.Valid()&&!worldSelected.Valid());}
    {Selection m1; m1.mode = 1; assert(!m1.Valid());
     assert(!m1.Match(0x4FAEF2389750DBFCull, 0x2FEC493CD52D7D78ull, 2908, 336));
     m1.vs=0x4FAEF2389750DBFCull;m1.ps=0x2FEC493CD52D7D78ull;m1.vsBytes=2908;m1.psBytes=336;
     assert(m1.Valid());
     assert(m1.Match(0x4FAEF2389750DBFCull, 0x2FEC493CD52D7D78ull, 2908, 336));
     assert(!m1.Match(0x488BAEB579E71A73ull, 0x27DFE7D9C67BCC03ull, 2968, 452));
     assert(!m1.Match(0x4FAEF2389750DBFCull, 0x2FEC493CD52D7D78ull, 2908, 340));
     m1.vs=0;m1.vsBytes=0;assert(!m1.Valid());}
    {selected={0x4FAEF2389750DBFCull,0x2FEC493CD52D7D78ull,2908,336,1,-1};
     vs.hash=selected.vs;vs.bytes=selected.vsBytes;ps.hash=selected.ps;ps.bytes=selected.psBytes;
     InterlockedExchange(&pathWitness,0);PathObserve(1,false);assert(pathCounts[1][0]==0);
     InterlockedExchange(&pathWitness,1);PathObserve(1,false);PathObserve(1,true);
     PathObserve(4,false);ps.hash=0;PathObserve(1,false);
     assert(pathCounts[1][0]==1&&pathCounts[1][1]==1);
     InterlockedExchange(&pathWitness,0);InterlockedExchange(&pathCounts[1][0],0);
     InterlockedExchange(&pathCounts[1][1],0);selected={};vs={};ps={};}
    {FakeShader shader;selected={};probe=false;assert(!Describe(&shader,true).hash&&!shader.queries);
        probe=true;auto first=Describe(&shader,true);assert(first.camera==12&&first.bytes==88&&shader.refs==2);
        assert(Describe(&shader,true).hash==first.hash&&shader.queries==1&&used==1);
        Release();assert(shader.refs==1&&used==0);
        shader.fail=true;assert(!Describe(&shader,true).hash&&shader.refs==2);
        const unsigned old=shader.queries;assert(!Describe(&shader,true).hash&&shader.queries==old);
        Release();assert(shader.refs==1);probe=false;
    }
    for(unsigned test=0;test<10;++test){Dev d;Slot slots[3]{};int n=0;Init(d,slots,n);Slot original[3];memcpy(original,slots,sizeof(slots));
        if(test==0)selected.mode=0;if(test==1)context=false;if(test==2)ps.hash=23;
        if(test==3)d.matrix[0]=2;if(test==4)vs.camera=-1;if(test==5)vs.camera=0;
        if(test==6)d.failRead=true;if(test==7)d.matrix[0]=NAN;
        {Scope scope(&d,slots,n,test==8?1:3,&Upload,true);
            assert(scope.changed==(test==9));
            if(scope.changed){assert(n==2&&slots[1].reg==12&&slots[1].conv==1);d.matrix[0]=999;}}
        assert(n==1&&!memcmp(slots,original,sizeof(slots)));
        assert(d.writes==(test==9?1u:0u));if(test==9)assert(d.matrix[0]==1);
        if(test<3||test==4)assert(!d.reads);
    }
    {Dev d;Slot s[3]{};int n;Init(d,s,n);s[1].reg=8;s[1].valid=false;n=2;Slot old=s[1];
        {Scope x(&d,s,n,3,&Upload,true);assert(x.changed&&n==2);}
        assert(n==2&&!memcmp(&s[1],&old,sizeof(old)));}
    {Dev d;Slot s[3]{};int n;Init(d,s,n);selected.convention=0;
        {Scope x(&d,s,n,3,&Upload,true);assert(!x.changed);}assert(!d.writes&&n==1);}
    {Dev d;Slot s[3]{};int n;Init(d,s,n);d.failWrite=true;{Scope x(&d,s,n,3,&Upload,true);assert(x.changed);}
        assert(fault&&n==1);const unsigned old=d.reads;{Scope x(&d,s,n,3,&Upload,true);assert(!x.changed);}assert(d.reads==old);}
    CapturePlan plan;for(unsigned i=0;i<89;++i)assert(!plan.Tick(true,0,false));assert(plan.Tick(true,0,false)==1);
    assert(!plan.Tick(true,0,false));assert(plan.Tick(true,1,false)==2);
    assert(!plan.Tick(false,2,false));assert(!plan.Tick(true,2,false));
    assert(plan.Tick(true,3,false)==2);assert(plan.Tick(true,4,false)==2);assert(!plan.Tick(true,5,false));
    CapturePlan interrupted;for(unsigned i=0;i<80;++i)assert(!interrupted.Tick(true,0,false));
    assert(!interrupted.Tick(true,1,false));for(unsigned i=0;i<89;++i)assert(!interrupted.Tick(true,1,false));
    assert(interrupted.Tick(true,1,false)==1);
    // Relative/overlapping data never qualifies just because it resembles a matrix.
    Slot slots[2]{};slots[0].valid=true;slots[0].reg=11;float m[16]{};assert(Alias(m,12,slots,1)<0);
    std::puts("PASS native muzzle: exact shader identity, hash parsing, alias isolation/restoration/failure, bounded idle/fire plan; no GPU");
}
