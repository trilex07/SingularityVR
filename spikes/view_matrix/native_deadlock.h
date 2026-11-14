#pragma once

// XR produces a one-shot intent. Only the existing UE3 input poll may consume it.
namespace NativeDeadlock {
constexpr ULONGLONG kFreshMs = 120, kPrepareMs = 150;
struct Input { ULONGLONG sample=0, request=0; uintptr_t pawn=0; };
static SRWLOCK lock=SRWLOCK_INIT;
static Input input{};
static void Publish(bool eligible, uintptr_t pawn=0, ULONGLONG now=0, bool release=false){
    AcquireSRWLockExclusive(&lock);
    if(!eligible||!pawn) input={};
    else {
        if(pawn!=input.pawn) input={};
        input.sample=now;input.pawn=pawn;
        if(release) input.request=now;
    }
    ReleaseSRWLockExclusive(&lock);
}
static Input Take(){
    AcquireSRWLockExclusive(&lock);
    const auto snapshot=input;
    input.request=0;
    ReleaseSRWLockExclusive(&lock);
    return snapshot;
}
struct Owner { uintptr_t pawn=0,tmd=0,power=0,handler=0; };
static Owner pending{};
static ULONGLONG deadline=0;
static bool fault=false;
static unsigned receipts=0;
static void Report(const char* stage,int detail=0){
    if(receipts++<32)Log("Deadlock native gesture: %s (%d)",stage,detail);
}

static bool Resolve(uintptr_t expected,Owner& o){
    using namespace NativeOwnedInventory;
    o.tmd=NativeTmdAim::LocalTmd(o.pawn);
    if(!o.tmd||o.pawn!=expected)return false;
    const auto tc=P(o.tmd+OBJ_CLASS),set=P(o.tmd+0x4d0),sc=P(set+OBJ_CLASS);
    static MemberCache setMember, heldMember, indexMember, firingMember, roleMember;
    if(setMember.Get(tc,"mPowerSet")!=0x4d0||heldMember.Get(tc,"mDeadlock")!=0x538||
       firingMember.Get(tc,"mFiringPowers")!=0x4d4||roleMember.Get(tc,"Role")!=0x92||
       !set||!ClassHas(sc,"RvTMDPowerSet")||indexMember.Get(sc,"mIndexedPowers")!=0x48||
       !PauseRttProbe::SafeRange(set+0x48,12))return false;
    const auto data=P(set+0x48);
    const int n=*(const int*)(set+0x4c),cap=*(const int*)(set+0x50);
    if(n<=3||n>32||cap<n||cap>128||!PauseRttProbe::SafeRange(data,size_t(n)*4))return false;
    o.power=P(data+3*4);
    if(!o.power||!ClassHas(P(o.power+OBJ_CLASS),"RvTMDPower_Deadlock")||
       !PauseRttProbe::SafeRange(o.power+0x44,0x48)||*(const BYTE*)(o.power+0x44)!=3)return false;
    const auto vt=P(o.power);
    o.handler=P(vt+0x128);
    return PauseRttProbe::SafeRange(o.tmd+0x54c,4)&&
        PauseRttProbe::SafeRange(o.tmd+0x92,1)&&*(const BYTE*)(o.tmd+0x92)==3; // ROLE_Authority (SP)
}
static bool Active(const Owner& o){
    using namespace NativeOwnedInventory;
    if(!PauseRttProbe::SafeRange(o.tmd+0x4d4,12))return false;
    const auto data=P(o.tmd+0x4d4);
    const int count=*(const int*)(o.tmd+0x4d8),cap=*(const int*)(o.tmd+0x4dc);
    if(count<=0||count>32||cap<count||cap>128||!PauseRttProbe::SafeRange(data,size_t(count)*4))return false;
    for(int i=0;i<count;++i)if(P(data+i*4)==o.power)return true;
    return false;
}
static bool TimerReady(const Owner& o){
    const auto value=*(const float*)(o.tmd+0x54c);
    return std::isfinite(value)&&value<=0.f;
}
static bool ArchetypeReady(const Owner& o){
    using namespace NativeOwnedInventory;
    const auto archetype=P(o.power+0x88);
    return archetype&&ClassHas(P(archetype+OBJ_CLASS),"RvProp_DeadlockGrenade")&&
        PauseRttProbe::SafeRange(archetype+0x3c0,4)&&P(archetype+OBJ_CLASS);
}
static bool LaunchContextReady(const Owner& o){
    using namespace NativeOwnedInventory;
    const auto held=P(o.tmd+0x538);
    // On authority, the original handler spawns its own grenade if no usable
    // prop is held. Never call its spawn branch from a client.
    if(!held)return ArchetypeReady(o);
    if(!ClassHas(P(held+OBJ_CLASS),"RvProp_DeadlockGrenade")||
       !PauseRttProbe::SafeRange(held+0xb0,4))return false;
    if(*(const uint32_t*)(held+0xb0)&0x400000u)return ArchetypeReady(o); // native respawn path
    return IsLive(held)&&ArchetypeReady(o)&&PauseRttProbe::SafeRange(held+0x3c0,4)&&
        PauseRttProbe::SafeRange(P(held+0x3b0)+0xac,1)&&P(P(held+0x180))!=0;
}
using Prepare=void (__thiscall*)(void*,void*,int);
using Launch=int (__thiscall*)(void*,void*,unsigned,int);
static bool StartStopContracts(){
    using namespace NativeTmdHaptics;
    if(InterlockedCompareExchange(&hooksReady,0,0))return originalStart&&originalStop;
    return NativeContextCursor::Method(kStartAddr,kStartSize,kStartHash)&&
           NativeContextCursor::Method(kStopAddr,kStopSize,kStopHash);
}
static bool Contracts(const Owner& o){
    using namespace NativeOwnedInventory;
    // Includes dispatch table and the usual eligibility, unlock and Start/Stop path.
    // Call that path instead of calling Start directly and skipping its eligibility test.
    return (uintptr_t)GetModuleHandleW(nullptr)==0x400000u&&StartStopContracts()&&
        NativeContextCursor::Method(0x100e100,0x150,0x2CAAA6366DC0FC7Eull)&&
        NativeContextCursor::Method(P(P(o.power)+0x11c),261,0x27E45AD8EECB4131ull)&&
        NativeContextCursor::Method(o.handler,1504,0xB26B7A29EA360695ull);
}
static void Stop(const Owner& o){
    using namespace NativeTmdHaptics;
    if(!Active(o)||!StartStopContracts())return;
    const auto fn=InterlockedCompareExchange(&hooksReady,0,0)?&NativeTmdHaptics::Stop:(StopFn)kStopAddr;
    fn((void*)o.power,nullptr,(void*)o.tmd);
}
static bool Fresh(ULONGLONG stamp,ULONGLONG now){
    return stamp&&now>=stamp&&now-stamp<=kFreshMs;
}
static void Tick(){
    const auto sample=Take();
    const auto now=GetTickCount64();
    const bool game=VrModeOn()&&InGameplay()&&!Rd(g_gamePaused)&&!Rd(g_vrMenuClass)&&
        !Rd(g_vrUiMenu)&&!Rd(g_panelOpen)&&!Rd(g_weaponWheelOpen)&&!Rd(g_inCinematic)&&
        !CutsceneNativeCameraArmed();
    if(!pending.power&&(!game||!Fresh(sample.request,now)||!Fresh(sample.sample,now)))return;
    Owner o{};
    if(!Resolve(pending.power?pending.pawn:sample.pawn,o)||
       (pending.power&&(o.tmd!=pending.tmd||o.power!=pending.power||o.handler!=pending.handler))){
        pending={};Report("local power unavailable or owner changed");return;
    }
    if(pending.power&&(!game||sample.pawn!=o.pawn||!Fresh(sample.sample,now)||now>deadline)){
        pending={};
        Stop(o);
        Report("preparation cancelled");
        return;
    }
    if(!pending.power){
        if(!Contracts(o)){Report("native contract refused");return;}
        if(!ArchetypeReady(o)||Active(o)||!TimerReady(o)){Report("native power occupied");return;}
        ((Prepare)0x100e100)((void*)o.tmd,(void*)o.power,1);
        if(!Active(o)){Report("native preparation refused");return;}
        pending=o;deadline=now+kPrepareMs;
    }
    if(!Active(o)){pending={};Report("native preparation ended");return;}
    if(!TimerReady(o)||!LaunchContextReady(o))return;
    pending={}; // never replay the native throw, including if the handler rejects it
    const int handled=((Launch)o.handler)((void*)o.power,(void*)o.tmd,0x1d,1);
    if(!handled)Stop(o);
    Report(handled?"native throw accepted":"native throw refused",handled);
}
static void Poll(){
    if(!g_tmdDeadlockGesture||fault)return;
    __try { Tick(); }
    __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){
        fault=true;pending={};Report("access violation; gesture disabled for this process");
    }
}
}
