// Synthetic own-process ABI/forwarding test; never loads the game or a proxy.
#include <windows.h>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cstdint>
volatile LONG g_tmdNativeAim=1,g_gamePaused=0,g_inCinematic=0,g_vrMenuClass=0,
    g_vrUiMenu=0,g_panelOpen=0,g_offHandValid=1,g_tmdOnOffHand=1,g_fireDepth=0;
int g_upNext=64;
constexpr int OBJ_CLASS=32;
static LONG Rd(volatile LONG& p){return InterlockedCompareExchange(&p,0,0);}
static bool VrModeOn(){return true;}
static bool TmdRaised(){return true;}
static bool IsLive(uintptr_t p){return p!=0;}
static void Log(const char*,...){}
struct LocalPlayer {uintptr_t pawn;};
static LocalPlayer ReadNativeLocalPlayer(){return {10};}
namespace PauseRttProbe {static bool SafeRange(uintptr_t p,size_t){return p!=0;}}
namespace NativeOwnedInventory {
static bool badContext=false;
static uintptr_t P(uintptr_t a){
    if(badContext)RaiseException(EXCEPTION_ACCESS_VIOLATION,0,0,nullptr);
    if(a==10+OBJ_CLASS)return 20;
    if(a==1000+OBJ_CLASS)return 30;
    if(a==18)return 1000;
    if(a==1000+0x98)return 10;
    return 0;
}
static bool Named(uintptr_t c,const char* n){return (c==20&&!std::strcmp(n,"RvPlayerPawnSP"))||
    (c==30&&!std::strcmp(n,"RvTMD"));}
struct MemberCache {int Get(uintptr_t,const char*){return 8;}};
}
namespace NativeContextCursor {static bool Method(uintptr_t,size_t,unsigned long long){return false;}}
constexpr int MH_OK=0;
static int MH_CreateHook(void*,void*,void**){return 1;}
static int MH_RemoveHook(void*){return 0;}
static int MH_EnableHook(void*){return 1;}
#include "../native_tmd_aim.h"
static unsigned calls=0,agingCalls=0;
static bool innerUntouched=false;
static void __fastcall NativeAim(void*,void*,float* start,float* dir){
    ++calls;for(unsigned i=0;i<3;++i){start[i]=float(i+7);dir[i]=i==0?1.f:0.f;}
}
static void __fastcall NativeAging(void* self,void*,float* start,float* dir){
    ++agingCalls;
    NativeTmdAim::Aim((void*)(uintptr_t(self)-0x1e0),nullptr,start,dir);
    innerUntouched=start[0]==7&&dir[0]==1&&dir[1]==0;
    start[0]=70;
}
static void __fastcall ThrowingAging(void*,void*,float*,float*) {
    RaiseException(EXCEPTION_INT_DIVIDE_BY_ZERO,0,0,nullptr);
}
static void TestUnwind(){
    float o[3]{},d[3]{};bool caught=false;
    NativeTmdAim::originalAging=ThrowingAging;
    __try {NativeTmdAim::Aging((void*)(1000+0x1e0),nullptr,o,d);}
    __except(EXCEPTION_EXECUTE_HANDLER){caught=true;}
    assert(caught&&NativeTmdAim::agingDepth==0);
}
int main(){
    g_tmdNativeAim=0;NativeTmdAim::Poll();g_tmdNativeAim=1;
    NativeTmdAim::originalAim=NativeAim;NativeTmdAim::originalAging=NativeAging;
    NativeTmdAim::engineThread=GetCurrentThreadId();
    NativeTmdAim::installedClass=30;
    InterlockedExchange(&NativeTmdAim::hooksReady,1);
    tmdaim::Pose sample;sample.pawn=10;sample.time=GetTickCount64();
    sample.matrix[0][1]=1;sample.matrix[1][0]=-1;sample.matrix[2][2]=1;sample.matrix[3][3]=1;
    sample.anchor[0]=10;sample.camera[0]=100;sample.forward[0]=1;
    NativeTmdAim::Publish(sample);
    float o[3]{},d[3]{};
    NativeTmdAim::Aim((void*)1000,nullptr,o,d);
    assert(calls==1&&o[0]==100&&o[1]==10&&d[1]==1);
    NativeTmdAim::Aging((void*)(1000+0x1e0),nullptr,o,d);
    assert(calls==2&&agingCalls==1&&innerUntouched&&o[0]==100&&d[1]==1);
    NativeTmdAim::Aim((void*)1001,nullptr,o,d);assert(calls==3&&o[0]==7&&d[0]==1);
    g_tmdNativeAim=0;NativeTmdAim::Aim((void*)1000,nullptr,o,d);assert(calls==4&&o[0]==7);
    g_tmdNativeAim=1;g_fireDepth=1;NativeTmdAim::Aim((void*)1000,nullptr,o,d);assert(calls==5&&o[0]==7);
    g_fireDepth=0;g_gamePaused=1;NativeTmdAim::Aim((void*)1000,nullptr,o,d);assert(calls==6&&o[0]==7);
    g_gamePaused=0;g_inCinematic=1;NativeTmdAim::Aim((void*)1000,nullptr,o,d);assert(calls==7&&o[0]==7);
    g_inCinematic=0;g_offHandValid=0;NativeTmdAim::Aim((void*)1000,nullptr,o,d);assert(calls==8&&o[0]==7);
    g_offHandValid=1;NativeTmdAim::engineThread=GetCurrentThreadId()+1;
    NativeTmdAim::Aim((void*)1000,nullptr,o,d);assert(calls==9&&o[0]==7);
    NativeTmdAim::engineThread=GetCurrentThreadId();sample.time=1;NativeTmdAim::Publish(sample);
    NativeTmdAim::Aim((void*)1000,nullptr,o,d);assert(calls==10&&o[0]==7);
    sample.time=GetTickCount64();NativeTmdAim::Publish(sample);
    InterlockedExchange(&NativeTmdAim::hooksReady,0);
    NativeTmdAim::Aim((void*)1000,nullptr,o,d);assert(calls==11&&o[0]==7);
    InterlockedExchange(&NativeTmdAim::hooksReady,1);
    float overlap[4]{},expected[4]{};
    NativeAim(nullptr,nullptr,expected,expected+1);
    NativeTmdAim::Aim((void*)1000,nullptr,overlap,overlap+1);
    assert(!std::memcmp(overlap,expected,sizeof(overlap)));
    TestUnwind();
    NativeOwnedInventory::badContext=true;
    NativeTmdAim::Aim((void*)1000,nullptr,o,d);
    assert(!g_tmdNativeAim&&o[0]==7&&d[0]==1);
    std::puts("PASS TMD native adapter: original forwarded exactly once; local-only outputs, nested aging, SEH unwind, OFF/pause/cinema/stale/invalid/gun/thread refusal");
}
