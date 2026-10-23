#pragma once
#include "dual_tmd_gun_scale_policy.h"
#include <intrin.h>

// Read-only result override at the ORIGINAL bone-scale producer. Not a shader
// palette patch, pose cache, pawn-state write or change to TMD power execution.
namespace NativeDualTmdGunScale {
using Getter=float (__thiscall*)(void*,int,void*);
static Getter original=nullptr;
static DWORD engineThread=0;
static uintptr_t target=0, control=0, localPawn=0, localMesh=0, controlClass=0;
static int meshOffset=-1, nameOffset=-1;
static unsigned receipts=0, attempts=0;
static int lastState=-1;
static volatile LONG refusals=0;
static ULONGLONG retry=0;
static bool Enabled(){return Rd(g_dualWeaponTmdGunScale)!=0;}
static void Refuse(LONG bit,const char* reason) {
    if(!(Rd(refusals)&bit)&&!(InterlockedOr(&refusals,bit)&bit))
        Log("dual-tmd gun-scale: refused %s; native result retained",reason);
}
static dualgunscale::Context Context() {
    // Avoid even evaluating scene state on the default-OFF path.
    if(!Enabled())return {};
    const bool dual=DualWeaponTmdContext();
    return {true,dual,dual&&NativeAnimatedHandsNow(),Rd(g_weaponHolstered)!=0,Rd(g_weaponObj)!=0};
}
static void Fault() {
    InterlockedExchange(&g_dualWeaponTmdGunScale,0);
    control=localPawn=localMesh=0;
    Refuse(1,"context AV (gate OFF)");
}
static bool NamedControl(uintptr_t c) {
    if(nameOffset<0||!PauseRttProbe::SafeRange(c+nameOffset,8))return false;
    const auto* n=(const int*)(c+nameOffset);char text[32]{};
    return n[1]==0&&NameFromIndex(n[0],text,sizeof(text))&&!strcmp(text,"gun_scale");
}
static float Resolve(void* self,int bone,void* mesh,uintptr_t caller,float value) {
    // Ordinary controls (clip, recoil, grenade, enemies) only take this cheap
    // rejection. Cached identity is not sufficient: recheck live ownership below.
    if(!Enabled()||uintptr_t(self)!=control)return value;
    if(GetCurrentThreadId()!=engineThread){Refuse(2,"animation thread differs from engine input thread");return value;}
    __try {
        using namespace NativeOwnedInventory;
        const auto c=Context();
        if(!dualgunscale::ContextAllowed(c))return value;
        const auto pawn=CameraOwnedPawn();
        const bool owned=pawn&&pawn==localPawn&&IsLive(control)&&
            P(pawn+dualgunscale::PawnControlOffset)==control&&P(control+OBJ_CLASS)==controlClass&&
            P(P(control)+dualgunscale::GetterSlot)==target&&NamedControl(control);
        const bool sameMesh=meshOffset>=0&&uintptr_t(mesh)==localMesh&&localMesh&&
            P(pawn+meshOffset)==localMesh&&IsLive(localMesh);
        if(!dualgunscale::Eligible(c,true,owned,sameMesh,caller,bone,value)){
            Refuse(!owned?4:!sameMesh?8:16,!owned?"live control ownership":!sameMesh?"inview component ownership":"caller/bone/native scale contract");
            return value;
        }
        const int state=(int)Rd(g_handsStatus)*2+(Rd(g_rawAltTrigger)>128?1:0);
        if(state!=lastState&&receipts<12){lastState=state;++receipts;
            Log("dual-tmd gun-scale: local gun_scale scale=0 -> 1 bone=%d pawn=%08X mesh=%08X control=%08X hands=%ld L2=%ld [live animation; no palette/pawn writes]",
                bone,(unsigned)pawn,(unsigned)localMesh,(unsigned)control,Rd(g_handsStatus),Rd(g_rawAltTrigger));}
        return 1.f;
    } __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) {
        Fault();return value;
    }
}
static __declspec(noinline) float __fastcall Hook(void* self,void*,int bone,void* mesh) {
    const auto caller=uintptr_t(_ReturnAddress());
    const float native=original(self,bone,mesh); // x87 result, ret8; exactly once
    return Resolve(self,bone,mesh,caller,native);
}
static void Poll() {
    if(!Enabled())return;
    if(engineThread&&engineThread!=GetCurrentThreadId())return;
    __try {
        using namespace NativeOwnedInventory;
        if(!dualgunscale::ContextAllowed(Context())) {control=localPawn=localMesh=0;return;}
        const auto pawn=CameraOwnedPawn();
        if(!pawn){control=localPawn=localMesh=0;Refuse(32,"local camera-owned pawn unavailable");return;}
        // Refresh a live instance when loading/switching animations, without
        // retaining UObject ownership across a level change or GC.
        const auto now=GetTickCount64();
        if(pawn==localPawn&&control&&P(pawn+dualgunscale::PawnControlOffset)==control&&
           meshOffset>=0&&P(pawn+meshOffset)==localMesh)return;
        control=localPawn=localMesh=0;
        if(now<retry)return;retry=now+1000;
        const auto pc=P(pawn+OBJ_CLASS);
        const int off=PropOffsetInChain(pc,"mGunScaleCtrl");
        // PostBeginPlay finds gun_scale on mInviewMesh, NOT Pawn.Mesh (body).
        meshOffset=PropOffsetInChain(pc,"mInviewMesh");
        if(off!=dualgunscale::PawnControlOffset||meshOffset<0||!NativeTmdAim::ExactClass(pc,"RvPlayerPawnSP")){
            Refuse(64,"reflected mGunScaleCtrl/mInviewMesh/local SP class");return;}
        const auto node=P(pawn+off),mesh=P(pawn+meshOffset),cls=P(node+OBJ_CLASS);
        if(!node||!mesh||!IsLive(node)||!IsLive(mesh)||!Named(cls,"SkelControlSingleBone")){
            Refuse(128,"live SingleBone control/inview mesh unavailable");return;}
        nameOffset=PropOffsetInChain(cls,"ControlName");
        if(PropOffsetInChain(cls,"BoneScale")!=dualgunscale::ScaleOffset||!NamedControl(node)){
            Refuse(256,"reflected BoneScale/ControlName");return;}
        const auto getter=P(P(node)+dualgunscale::GetterSlot);
        if((uintptr_t)GetModuleHandleW(nullptr)!=0x400000||getter!=dualgunscale::Getter||
           !NativeContextCursor::Method(dualgunscale::Consumer,dualgunscale::ConsumerBytes,dualgunscale::ConsumerHash)){
            Refuse(512,"executable base/vtable/consumer fingerprint");return;}
        if(!target) {
            if(!NativeContextCursor::Method(getter,dualgunscale::GetterBytes,dualgunscale::GetterHash)||attempts++>=2){
                Refuse(1024,"getter fingerprint or hook retry budget");return;}
            if(MH_CreateHook((void*)getter,(void*)&Hook,(void**)&original)!=MH_OK){Refuse(2048,"hook creation");return;}
            engineThread=GetCurrentThreadId();target=getter;
            if(MH_EnableHook((void*)getter)!=MH_OK) {
                // A trampoline may already be callable. Retain it, disable only our override.
                InterlockedExchange(&g_dualWeaponTmdGunScale,0);Refuse(4096,"hook enable (gate OFF)");return;
            }
        }
        controlClass=cls;localPawn=pawn;localMesh=mesh;control=node;
        if(receipts<12){++receipts;Log("dual-tmd gun-scale: ready reflected mGunScaleCtrl=1024 BoneScale=7C getter=%08X pawn=%08X mesh=%08X control=%08X [default-OFF override; physical validation pending]",
            (unsigned)target,(unsigned)pawn,(unsigned)mesh,(unsigned)node);}
    } __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) {Fault();}
}
}
