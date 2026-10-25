#pragma once
#include "tmd_aim_policy.h"
// Original GetAimVectors(out FVector start,out FVector direction), ret8.
// Aging interface has the same ABI but this points to TMD+0x1e0. Offline
// audit_tmd_aim.py proves both full bodies and the nested call relationship.
namespace NativeTmdAim {
using Getter=void (__fastcall*)(void*,void*,float*,float*);
static Getter originalAim=nullptr,originalAging=nullptr;
static uintptr_t aimTarget=0,agingTarget=0,installedClass=0;
static DWORD engineThread=0;
static SRWLOCK poseLock=SRWLOCK_INIT;
static tmdaim::Pose pose{};
static thread_local unsigned agingDepth=0;
static volatile LONG hooksReady=0;
static bool Enabled(){return InterlockedCompareExchange(&g_tmdNativeAim,0,0)!=0;}
static void Fault(const char* site) {
    InterlockedExchange(&g_tmdNativeAim,0);
    static unsigned notes=0;
    if(notes<4){++notes;Log("TMD native aim: unreadable %s context; gate OFF, native forwarding retained",site);}
}
static void Publish(const tmdaim::Pose& next) {
    if(TryAcquireSRWLockExclusive(&poseLock)) {
        pose=next;ReleaseSRWLockExclusive(&poseLock);
    }
}
static bool ExactClass(uintptr_t cls,const char* expected) {
    for(unsigned depth=0;cls&&g_upNext>0&&depth<32;++depth,cls=NativeOwnedInventory::P(cls+g_upNext-4))
        if(NativeOwnedInventory::Named(cls,expected))return true;
    return false;
}
static uintptr_t LocalTmd(uintptr_t& pawn) {
    using namespace NativeOwnedInventory;
    // Reciprocal native controller.Pawn, rather than an optional camera owner.
    // The failed physical session had a valid native pawn and cameraPawn=0.
    pawn=ReadNativeLocalPlayer().pawn;
    if(!pawn||!ExactClass(P(pawn+OBJ_CLASS),"RvPlayerPawnSP"))return 0;
    static MemberCache member;
    const int off=member.Get(P(pawn+OBJ_CLASS),"mTMD");
    const auto tmd=off>=0?P(pawn+off):0;
    if(!tmd||!IsLive(tmd)||!ExactClass(P(tmd+OBJ_CLASS),"RvTMD"))return 0;
    // Native GetTMDOwner getter is independently fingerprinted at installation.
    return P(tmd+0x98)==pawn?tmd:0;
}
static void Rewrite(void* self,float* start,float* direction,bool aging) {
    if(!Enabled()||engineThread!=GetCurrentThreadId()||!InterlockedCompareExchange(&hooksReady,0,0))return;
    __try {
        const bool game=VrModeOn()&&!Rd(g_gamePaused)&&!Rd(g_inCinematic)&&
            !Rd(g_vrMenuClass)&&!Rd(g_vrUiMenu)&&!Rd(g_panelOpen)&&TmdRaised();
        if(!game||!Rd(g_offHandValid)||!Rd(g_tmdOnOffHand)||Rd(g_fireDepth)>0)return;
        uintptr_t pawn=0;const auto tmd=LocalTmd(pawn);
        const bool local=tmd&&installedClass&&NativeOwnedInventory::P(tmd+OBJ_CLASS)==installedClass&&
            uintptr_t(self)==tmd+(aging?0x1e0:0);
        if(!tmdaim::Eligible(true,game,Rd(g_offHandValid)!=0&&Rd(g_tmdOnOffHand)!=0,
            local,true,Rd(g_fireDepth)>0)||!PauseRttProbe::SafeRange((uintptr_t)start,12)||
            !PauseRttProbe::SafeRange((uintptr_t)direction,12))return;
        const auto s=(uintptr_t)start,d=(uintptr_t)direction;
        if((s>d?s-d:d-s)<12)return; // overlapping out references: leave native result intact
        tmdaim::Pose sample;
        if(!TryAcquireSRWLockShared(&poseLock))return;
        sample=pose;ReleaseSRWLockShared(&poseLock);
        if(!tmdaim::Current(sample,pawn,GetTickCount64()))return;
        float origin[3],axis[3];if(!tmdaim::Ray(sample,origin,axis))return;
        // Native owns the out buffers and has already filled them. No UObject,
        // camera, inventory, shared gun aim or muzzle mailbox writes.
        for(unsigned i=0;i<3;++i){start[i]=origin[i];direction[i]=axis[i];}
        static unsigned receipts[2]{};auto& n=receipts[aging?1:0];
        if(n<12){++n;Log("TMD native aim: %s pawn=%08X tmd=%08X age=%llums origin=(%.3f,%.3f,%.3f) direction=(%.6f,%.6f,%.6f) [getter output only; not pixel proof]",
            aging?"aging":"aim",(unsigned)pawn,(unsigned)tmd,
            (unsigned long long)(GetTickCount64()-sample.time),origin[0],origin[1],origin[2],axis[0],axis[1],axis[2]);}
    } __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) {
        Fault("getter");
    }
}
static void __fastcall Aim(void* self,void* edx,float* start,float* direction) {
    originalAim(self,edx,start,direction);
    if(!agingDepth)Rewrite(self,start,direction,false);
}
static void __fastcall Aging(void* self,void* edx,float* start,float* direction) {
    ++agingDepth;
    __try { originalAging(self,edx,start,direction); }
    __finally { --agingDepth; }
    Rewrite(self,start,direction,true);
}
static void Poll() {
    if(!Enabled())return;
    if(Rd(g_gamePaused)||Rd(g_inCinematic)||Rd(g_vrMenuClass)||Rd(g_vrUiMenu)||Rd(g_panelOpen)||!VrModeOn()) {
        if(TryAcquireSRWLockExclusive(&poseLock)){pose.time=0;ReleaseSRWLockExclusive(&poseLock);}
        return;
    }
    if(aimTarget)return;
    static ULONGLONG retry=0;const auto now=GetTickCount64();
    if(now<retry)return;retry=now+2000;
    __try {
        using namespace NativeOwnedInventory;
        uintptr_t pawn=0;const auto tmd=LocalTmd(pawn);
        if(!tmd){static unsigned missing=0;if(missing<4){++missing;
            Log("TMD native aim: waiting for reflected local SP pawn.mTMD + owner; no hook/no writes");}return;}
        const auto vt=P(tmd),avt=P(tmd+0x1e0);
        const auto aim=P(vt+0x440),aging=P(avt+12),owner=P(vt+0x43c);
        const bool ownerOk=NativeContextCursor::Method(owner,7,0x940DBEC1332B5EEEull);
        const bool aimOk=NativeContextCursor::Method(aim,119,0x19B6BABB20248999ull);
        const bool agingOk=NativeContextCursor::Method(aging,59,0x73C3C6D548B0AF74ull);
        if(!ownerOk||!aimOk||!agingOk){static unsigned mismatches=0;if(mismatches<4){++mismatches;
            Log("TMD native aim: contract refused owner=%d aim=%d aging=%d; no hook/no writes",ownerOk?1:0,aimOk?1:0,agingOk?1:0);}return;}
        static unsigned attempts=0;if(attempts++>=2)return;
        if(MH_CreateHook((void*)aim,(void*)&Aim,(void**)&originalAim)!=MH_OK)return;
        if(MH_CreateHook((void*)aging,(void*)&Aging,(void**)&originalAging)!=MH_OK) {
            MH_RemoveHook((void*)aim);originalAim=nullptr;return;
        }
        engineThread=GetCurrentThreadId();installedClass=P(tmd+OBJ_CLASS);
        if(MH_EnableHook((void*)aim)!=MH_OK||MH_EnableHook((void*)aging)!=MH_OK) {
            // Keep callable trampolines after any enable attempt. Never null an
            // original while another thread might already be entering the detour.
            InterlockedExchange(&g_tmdNativeAim,0);
            Log("TMD native aim: enable failed, gate OFF; trampolines retained");return;
        }
        aimTarget=aim;agingTarget=aging;InterlockedExchange(&hooksReady,1);
        Log("TMD native aim: verified getters installed aim=%08X aging=%08X pawn=%08X [runtime/physical acceptance pending]",
            (unsigned)aimTarget,(unsigned)agingTarget,(unsigned)pawn);
    } __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) {
        Fault("installer");
    }
}
}
