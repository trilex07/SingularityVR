#pragma once
#include "dual_tmd_gun_scale_policy.h"
#include <intrin.h>

// The startup/local-player path independently verifies Camera.PCOwner,
// Controller.Pawn and Pawn.Controller. The older inventory reflection can
// temporarily miss the camera while that reciprocal path is already live.
std::uintptr_t EngineNativeLocalPlayerPawn();

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
static int lastL2Edge=-1, lastR2Edge=-1;
static unsigned edgeReports=0, resolverReports=0;
static uintptr_t lastResolverPawn=0;
static int lastControlOff=-3,lastInviewOff=-3;
static bool Enabled(){return Rd(g_dualWeaponTmdGunScale)!=0;}
static uintptr_t OwnedPawn() {
    const uintptr_t camera=NativeOwnedInventory::CameraOwnedPawn();
    const uintptr_t native=EngineNativeLocalPlayerPawn();
    // Never override another pawn's authored gun presentation if the two
    // independently verified owners disagree during a level transition.
    if(camera&&native&&camera!=native)return 0;
    return native?native:camera;
}
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
// The old context receipt sampled only changes of the dual/weapon state. It
// never sampled a held Impulse, and combined three unrelated reflection gates
// into one refusal. Capture the actual input edges and each producer identity
// without changing the native scale or retaining an UObject pointer.
static void Witness(const char* reason,uintptr_t pawn=0,int controlOff=-2,int inviewOff=-2) {
    if((!strcmp(reason,"edge") ? edgeReports++ : resolverReports++)>=16)return;
    using namespace NativeOwnedInventory;
    const uintptr_t cameraPawn=CameraOwnedPawn(),nativePawn=EngineNativeLocalPlayerPawn();
    const uintptr_t cachedPawn=g_playerPawn,cls=pawn?P(pawn+OBJ_CLASS):0;
    char className[80]{};
    if(cls)NameOf(cls,className,sizeof(className));
    const uintptr_t node=controlOff>=0&&pawn?P(pawn+controlOff):0;
    const uintptr_t mesh=inviewOff>=0&&pawn?P(pawn+inviewOff):0;
    Log("dual-tmd gun-scale witness: %s Impulse=%ld ADS=%ld R2=%ld hands=%ld dual=%d menu=%ld pause=%ld camera=%08X controller=%08X cachedPawn=%08X cameraPawn=%08X nativePawn=%08X selected=%08X class=%s controlOff=%d expected=1024 inviewOff=%d node=%08X mesh=%08X armed=%d",
        reason,Rd(g_rawImpulsePressed),Rd(g_rawAltTrigger),Rd(g_rawTrigger),Rd(g_handsStatus),DualWeaponTmdContext(),
        Rd(g_vrMenuClass),Rd(g_gamePaused),(unsigned)g_camera,(unsigned)g_controller,
        (unsigned)cachedPawn,(unsigned)cameraPawn,(unsigned)nativePawn,(unsigned)pawn,
        className,controlOff,inviewOff,(unsigned)node,(unsigned)mesh,target!=0);
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
        const auto pawn=OwnedPawn();
        const bool owned=pawn&&pawn==localPawn&&IsLive(control)&&
            P(pawn+dualgunscale::PawnControlOffset)==control&&P(control+OBJ_CLASS)==controlClass&&
            P(P(control)+dualgunscale::GetterSlot)==target&&NamedControl(control);
        const bool sameMesh=meshOffset>=0&&uintptr_t(mesh)==localMesh&&localMesh&&
            P(pawn+meshOffset)==localMesh&&IsLive(localMesh);
        if(!dualgunscale::Eligible(c,true,owned,sameMesh,caller,bone,value)){
            Refuse(!owned?4:!sameMesh?8:16,!owned?"live control ownership":!sameMesh?"inview component ownership":"caller/bone/native scale contract");
            return value;
        }
        const int state=(int)Rd(g_handsStatus)*2+(Rd(g_rawImpulsePressed)?1:0);
        if(state!=lastState&&receipts<12){lastState=state;++receipts;
            Log("dual-tmd gun-scale: local gun_scale scale=0 -> 1 bone=%d pawn=%08X mesh=%08X control=%08X hands=%ld Impulse=%ld ADS=%ld [live animation; no palette/pawn writes]",
                bone,(unsigned)pawn,(unsigned)localMesh,(unsigned)control,Rd(g_handsStatus),Rd(g_rawImpulsePressed),Rd(g_rawAltTrigger));}
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
        const auto context=Context();
        const int l2=Rd(g_rawImpulsePressed)?1:0,r2=Rd(g_rawTrigger)>128?1:0;
        if(l2!=lastL2Edge||r2!=lastR2Edge){
            lastL2Edge=l2;lastR2Edge=r2;
            if(context.dual)Witness("edge",OwnedPawn());
        }
        const unsigned state=(context.dual?1u:0u)|(context.nativeHands?2u:0u)|
            (context.holstered?4u:0u)|(context.equipped?8u:0u);
        static unsigned previous=~0u, contextReceipts=0;
        if(state!=previous){previous=state;
            if(contextReceipts++<20)Log("dual-tmd gun-scale: context dual=%d nativeHands=%d holstered=%d equipped=%d hands=%ld Impulse=%ld ADS=%ld",
                context.dual,context.nativeHands,context.holstered,context.equipped,
                Rd(g_handsStatus),Rd(g_rawImpulsePressed),Rd(g_rawAltTrigger));}
        if(!dualgunscale::ContextAllowed(context)) {control=localPawn=localMesh=0;return;}
        const auto pawn=OwnedPawn();
        if(!pawn){control=localPawn=localMesh=0;
            if(!(Rd(refusals)&32))Witness("owner missing");
            if(!(Rd(refusals)&32))Log("dual-tmd gun-scale: owner sources camera=%08X native=%08X",
                (unsigned)CameraOwnedPawn(),(unsigned)EngineNativeLocalPlayerPawn());
            Refuse(32,"reciprocal local pawn unavailable or owners disagree");return;}
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
        if(pawn!=lastResolverPawn||off!=lastControlOff||meshOffset!=lastInviewOff){
            lastResolverPawn=pawn;lastControlOff=off;lastInviewOff=meshOffset;
            Witness("resolve pawn fields",pawn,off,meshOffset);
        }
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
