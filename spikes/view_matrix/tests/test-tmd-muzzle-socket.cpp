#define NOMINMAX
#include <windows.h>
#include <intrin.h>
#include <cassert>
#include <cstdio>
#include <limits>
#include "../tmd_muzzle_socket.h"
#include "../arm_natural.h"
static armbind::Mat Identity(){armbind::Mat m;for(unsigned i=0;i<4;++i)m.m[i][i]=1;return m;}
static armbind::Mat Product(const armbind::Mat& a,const armbind::Mat& b){armbind::Mat m;skeletal::Multiply(m.m,a.m,b.m);return m;}
static void Near(const armbind::Mat& a,const armbind::Mat& b){for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j)assert(std::fabs(a.m[i][j]-b.m[i][j])<.0002f);}
static bool g_tmdTrackedMuzzle=true;
static bool g_tmdTrackedBeam=false;
static LONG g_gamePaused=0,g_inCinematic=0,g_panelOpen=0,g_vrMenuClass=0,g_vrUiMenu=0,
    g_twoHandActive=0,g_tmdOnOffHand=1,g_offHandValid=1;
static bool VrModeOn(){return true;}
static bool TmdRaised(){return true;}
static LONG Rd(LONG& x){return x;}
static void Log(const char*,...){}
static bool IsLive(uintptr_t x){return x==300||x==600;}
constexpr unsigned OBJ_CLASS=32;
namespace PauseRttProbe {static bool SafeRange(uintptr_t p,size_t){return p!=0;}}
static bool NameFromIndex(int i,char* out,size_t){strcpy_s(out,96,i==7?"TMD_Muzzle":"shellEject");return true;}
namespace NativeOwnedInventory {
struct Name {int index,number;};
static bool fail=false;
static uintptr_t childReturn=0;
static uintptr_t P(uintptr_t p){if(fail)RaiseException(EXCEPTION_ACCESS_VIOLATION,0,0,nullptr);
    return p==200+OBJ_CLASS?400:p==200+16?300:p==300+OBJ_CLASS?500:
        p==200+24?600:p==600+OBJ_CLASS?700:p==300?800:p==800+0x138?childReturn-0x161:0;}
struct MemberCache {int Get(uintptr_t,const char* n){return !strcmp(n,"mAgingBeam")?24:16;}};
}
namespace NativeTmdAim {
static uintptr_t LocalTmd(uintptr_t& pawn){pawn=100;return 200;}
static bool ExactClass(uintptr_t c,const char* n){return (c==500&&!strcmp(n,"SkeletalMeshComponent"))||
    (c==700&&!strcmp(n,"RvCurvedBeamComponent"));}
}
namespace NativeRegistry {static uintptr_t Find(const char*){return 0;}}
namespace NativeContextCursor {static bool Method(uintptr_t,size_t,unsigned long long){return false;}}
namespace NativeRifleFx {static uintptr_t Helper(uintptr_t,unsigned,size_t,unsigned long long){return 0;}}
constexpr int MH_OK=0;
static int MH_CreateHook(void*,void*,void**){return 1;}
static int MH_EnableHook(void*){return 1;}
#include "../native_tmd_socket.inl"
#include "../native_tmd_beam.inl"
static unsigned componentCalls=0;
static const armbind::Mat* passedParent=nullptr;
static armbind::Mat nativeParent;
static void __fastcall ComponentUpdate(void* self,void*,void* scene,void* owner,const armbind::Mat* parent,int collision) {
    assert(self==(void*)600||self==(void*)601);assert(scene==(void*)901&&owner==(void*)902&&collision==17);
    assert(((uintptr_t)parent&15)==0);passedParent=parent;nativeParent=*parent;++componentCalls;
}
using ComponentHook=void (__fastcall*)(void*,void*,void*,void*,const armbind::Mat*,int);
static void __fastcall MeasureReturn(void*,void*,void*,void*,const armbind::Mat*,int) {
    NativeTmdBeam::attachmentReturn=(uintptr_t)_ReturnAddress();
    NativeOwnedInventory::childReturn=NativeTmdBeam::attachmentReturn;
}
// Both calibration and the REAL production callback execute at the same call
// instruction. A volatile write prevents a tailcall from hiding the ABI.
static volatile unsigned callReceipt=0;
__declspec(noinline) static void InvokeComponent(ComponentHook fn,void* self,const armbind::Mat* parent) {
    fn(self,nullptr,(void*)901,(void*)902,parent,17);++callReceipt;
}
static unsigned nativeCalls=0,converterCalls=0;
static int result=1;
static int __fastcall NativeQuery(void* self,void*,NativeOwnedInventory::Name name,float* p,int* r){
    assert(self==(void*)300||self==(void*)301);assert(name.index==7||name.index==8);
    ++nativeCalls;for(unsigned i=0;i<3;++i){p[i]=float(i+10);if(r)r[i]=int(i+20);}return result;
}
static int* __fastcall Convert(const void* self,void*,int* out){
    assert(armbind::Rigid(*(const armbind::Mat*)self));++converterCalls;
    out[0]=101;out[1]=202;out[2]=303;return out;
}
__declspec(noinline) static uintptr_t Esp(){uintptr_t s;
    __asm { mov s,esp }
    return s;
}
int main(){
    using namespace tmdmuzzle;
    const auto socket=SocketLocal();assert(armbind::Rigid(socket));
    assert(socket.m[3][0]==5.02409f&&socket.m[3][1]==-7.532f&&socket.m[3][2]==-.710843f);
    const auto& bind=armnatural::TmdBind();assert(bind.ready);
    Witness w, wristRotations[4];
    // Recover a known joint from the ORIGINAL bind and nontrivial GPU skin;
    // compare socket * joint * local * actual camera ride + world camera.
    for(unsigned turn=0;turn<4;++turn){
        const float angle=float(turn)*.7f;auto joint=Identity();
        joint.m[0][0]=joint.m[1][1]=std::cos(angle);joint.m[0][1]=std::sin(angle);joint.m[1][0]=-std::sin(angle);
        joint.m[3][0]=31; joint.m[3][1]=-22; joint.m[3][2]=17;
        armbind::Mat inverse;assert(skeletal::Inverse(inverse.m,bind.bind[9].m));
        const auto skin=Product(inverse,joint);float gpu[3][4];armbind::ToGpu(skin,gpu);
        auto local=Identity(),ride=Identity();local.m[3][2]=-73;ride.m[3][1]=19;
        const float camera[]{100,200,300};
        assert(Capture(bind.bind[9],gpu,local,ride,camera,100,GetTickCount64(),12,w));
        wristRotations[turn]=w;
        auto expected=Product(Product(Product(socket,joint),local),ride);
        for(unsigned i=0;i<3;++i)expected.m[3][i]+=camera[i];
        armbind::Mat actual;assert(Socket(w,100,GetTickCount64(),actual));Near(actual,expected);
        auto other=w;assert(SameEyes(w,other));other.wrist.m[3][0]+=.01f;assert(!SameEyes(w,other));
        other=w;other.frame++;assert(!SameEyes(w,other));other=w;other.pawn++;assert(!SameEyes(w,other));
        assert(!Socket(w,99,GetTickCount64(),actual));assert(!Socket(w,100,w.tick+151,actual));
        auto invalid=local;invalid.m[0][0]=std::numeric_limits<float>::quiet_NaN();
        const auto before=w;assert(!Capture(bind.bind[9],gpu,invalid,ride,camera,100,1,12,w));Near(w.wrist,before.wrist);
        assert(!Capture(bind.bind[9],gpu,local,armbind::Mat{},camera,100,1,12,w));
    }
    NativeTmdSocket::original=(NativeTmdSocket::Query)&NativeQuery;
    Witness eyes[2]{w,w};bool drawn[2]{true,true};
    assert(Completed(eyes,drawn,2,true,true,true,true));
    for(unsigned bit=0;bit<4;++bit)assert(!Completed(eyes,drawn,2,bit!=0,bit!=1,bit!=2,bit!=3));
    assert(!Completed(eyes,drawn,1,true,true,true,true));drawn[1]=false;
    assert(!Completed(eyes,drawn,2,true,true,true,true));drawn[1]=true;eyes[1].frame++;
    assert(!Completed(eyes,drawn,2,true,true,true,true));
    NativeTmdSocket::rotation=(NativeTmdSocket::Rotation)&Convert;
    NativeTmdSocket::thread=GetCurrentThreadId();w.tick=GetTickCount64();NativeTmdSocket::Publish(w);
    float p[3]{};int r[3]{};armbind::Mat expected;assert(Socket(w,100,GetTickCount64(),expected));
    const auto stack=Esp();
    for(unsigned i=0;i<100;++i){assert(NativeTmdSocket::Hook((void*)300,nullptr,{7,0},p,r)==1);
        assert(p[0]==expected.m[3][0]&&p[1]==expected.m[3][1]&&r[0]==101&&r[2]==303);}
    assert(Esp()==stack&&nativeCalls==100&&converterCalls==100);
    auto unchanged=[&](void* self,NativeOwnedInventory::Name name){const auto calls=nativeCalls,converts=converterCalls;
        assert(NativeTmdSocket::Hook(self,nullptr,name,p,r)==result);assert(p[0]==10&&r[0]==20);
        assert(nativeCalls==calls+1&&converterCalls==converts);};
    g_tmdTrackedMuzzle=false;NativeTmdSocket::Poll();unchanged((void*)300,{7,0});g_tmdTrackedMuzzle=true;
    unchanged((void*)301,{7,0});unchanged((void*)300,{8,0});unchanged((void*)300,{7,1});
    g_gamePaused=1;unchanged((void*)300,{7,0});g_gamePaused=0;
    g_twoHandActive=1;unchanged((void*)300,{7,0});g_twoHandActive=0;
    g_inCinematic=1;unchanged((void*)300,{7,0});g_inCinematic=0;
    NativeTmdSocket::thread++;unchanged((void*)300,{7,0});NativeTmdSocket::thread--;
    result=0;unchanged((void*)300,{7,0});result=1;
    w.tick=GetTickCount64()-151;NativeTmdSocket::Publish(w);unchanged((void*)300,{7,0});
    w.tick=GetTickCount64();NativeTmdSocket::Publish(w);
    const auto converts=converterCalls;NativeTmdSocket::Hook((void*)300,nullptr,{7,0},p,nullptr);
    assert(converterCalls==converts&&p[0]==expected.m[3][0]);
    NativeOwnedInventory::fail=true;unchanged((void*)300,{7,0});assert(!g_tmdTrackedMuzzle);
    NativeOwnedInventory::fail=false;
    // Reproduce the ACTUAL attachment dispatch, not the socket query: native
    // CPU parent enters a four-dword virtual call and is copied synchronously.
    NativeTmdBeam::original=(NativeTmdBeam::Update)&ComponentUpdate;
    NativeTmdBeam::thread=GetCurrentThreadId();g_tmdTrackedBeam=true;
    __declspec(align(16)) auto cpuParent=Identity();cpuParent.m[3][0]=-500;
    const auto savedParent=cpuParent;
    InvokeComponent(&MeasureReturn,(void*)600,&cpuParent);
    w.tick=GetTickCount64();NativeTmdSocket::Publish(w);
    assert(Socket(w,100,GetTickCount64(),expected));
    const auto componentStack=Esp();
    for(const auto& rotated:wristRotations){
        w=rotated;w.tick=GetTickCount64();NativeTmdSocket::Publish(w);
        assert(Socket(w,100,GetTickCount64(),expected));
        for(unsigned i=0;i<25;++i)InvokeComponent(&NativeTmdBeam::Hook,(void*)600,&cpuParent);
        Near(nativeParent,expected);Near(cpuParent,savedParent);
    }
    assert(componentCalls==100&&Esp()==componentStack&&passedParent!=&cpuParent);
    Near(nativeParent,expected);Near(cpuParent,savedParent);
    auto beamUnchanged=[&](void* self){const auto before=componentCalls;
        InvokeComponent(&NativeTmdBeam::Hook,self,&cpuParent);
        assert(componentCalls==before+1&&passedParent==&cpuParent);Near(nativeParent,cpuParent);};
    g_tmdTrackedBeam=false;NativeTmdBeam::Poll();beamUnchanged((void*)600);g_tmdTrackedBeam=true;
    beamUnchanged((void*)601);
    g_gamePaused=1;beamUnchanged((void*)600);g_gamePaused=0;
    g_twoHandActive=1;beamUnchanged((void*)600);g_twoHandActive=0;
    g_inCinematic=1;beamUnchanged((void*)600);g_inCinematic=0;
    g_offHandValid=0;beamUnchanged((void*)600);g_offHandValid=1;
    NativeTmdBeam::thread++;beamUnchanged((void*)600);NativeTmdBeam::thread--;
    NativeTmdBeam::attachmentReturn++;beamUnchanged((void*)600);NativeTmdBeam::attachmentReturn--;
    NativeOwnedInventory::childReturn++;beamUnchanged((void*)600);NativeOwnedInventory::childReturn--;
    w.tick=GetTickCount64()-151;NativeTmdSocket::Publish(w);beamUnchanged((void*)600);
    w.tick=GetTickCount64();w.pawn=99;NativeTmdSocket::Publish(w);beamUnchanged((void*)600);
    w.pawn=100;NativeTmdSocket::Publish(w);
    NativeOwnedInventory::fail=true;beamUnchanged((void*)600);assert(!g_tmdTrackedBeam);
    puts("PASS TMD aging attachment: real four-argument production callback x100 across 4 wrist rotations, preserved ESP/non-overwritten parent, aligned copy, exact owned child/caller, stale/pawn/tracking/support/cinema/AV/OFF fallback; original called once; no endpoint/power/energy writes");
    puts("PASS TMD socket: original bind/animated skin + final local/ride + native socket; 4 rotations, stereo identity, no double camera; real adapter ABI 100 calls ESP preserved, exact owner/name/thread/freshness/contexts, original once, no gameplay writes");
}
