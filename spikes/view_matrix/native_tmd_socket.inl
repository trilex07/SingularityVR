// Exact native socket query only: no particle recreation, power/energy writes,
// camera changes or global weapon trace edits. TMD_Muzzle remains native-owned.
namespace NativeTmdSocket {
using Name=NativeOwnedInventory::Name;
using Query=int (__thiscall*)(void*,Name,float*,int*);
using Rotation=int* (__thiscall*)(const void*,int*);
using GetMesh=void* (__cdecl*)(void*);
static Query original=nullptr;
static Query originalBone=nullptr;
static GetMesh originalMesh=nullptr;
static Rotation rotation=nullptr;
static DWORD thread=0;
static SRWLOCK lock=SRWLOCK_INIT;
static tmdmuzzle::Witness published;
static void Publish(const tmdmuzzle::Witness& w) {
    if(TryAcquireSRWLockExclusive(&lock)){published=w;ReleaseSRWLockExclusive(&lock);}
}
static void Note(const char* stage) {
    static const char* last=nullptr;static unsigned count=0;
    if(last==stage||count>=16)return;last=stage;++count;
    Log("TMD tracked muzzle: gate=%s [original query forwarded]",stage);
}
static bool Rewrite(void* mesh,Name name,float* position,int* rot) {
    if(!g_tmdTrackedMuzzle||GetCurrentThreadId()!=thread)return false;
    __try {
        if(!VrModeOn()||Rd(g_gamePaused)||Rd(g_inCinematic)||Rd(g_panelOpen)||
           Rd(g_vrMenuClass)||Rd(g_vrUiMenu)||Rd(g_twoHandActive)||!TmdRaised()||
           !Rd(g_tmdOnOffHand)||!Rd(g_offHandValid)){Note("context");return false;}
        char text[96]{};
        if(name.number||!NameFromIndex(name.index,text,sizeof(text)))return false;
        uintptr_t pawn=0;const auto tmd=NativeTmdAim::LocalTmd(pawn);
        // RvTMDPower_Age::CreateFX loads the actual owner component from the
        // audited RvTMD argument at +204h. Reflection did not resolve this
        // property in the physical census, so use the native read contract.
        const auto owned=tmd?NativeOwnedInventory::P(tmd+0x204):0;
        if(!owned||owned!=(uintptr_t)mesh||!IsLive(owned)||
           !NativeTmdAim::ExactClass(NativeOwnedInventory::P(owned+OBJ_CLASS),"SkeletalMeshComponent")) {
            Note("not-owned-TMD-component");return false;
        }
        // Bounded observation of other original socket names on the owned
        // component. They remain unmodified; a different beam source must not
        // silently inherit the TMD_Muzzle transform.
        if(strcmp(text,"TMD_Muzzle")) {
            static int seen[8]{};static unsigned count=0;bool found=false;
            for(unsigned i=0;i<count;++i)if(seen[i]==name.index)found=true;
            if(!found&&count<8){seen[count++]=name.index;
                Log("TMD tracked muzzle: other original owned socket=%s mesh=%p unchanged",text,mesh);}
            return false;
        }
        if(!PauseRttProbe::SafeRange((uintptr_t)position,12)||
           (rot&&!PauseRttProbe::SafeRange((uintptr_t)rot,12)))return false;
        const auto a=(uintptr_t)position,b=(uintptr_t)rot;
        if(rot&&(a>b?a-b:b-a)<12)return false;
        tmdmuzzle::Witness w;
        if(!TryAcquireSRWLockShared(&lock))return false;
        w=published;ReleaseSRWLockShared(&lock);
        armbind::Mat socket;
        if(!tmdmuzzle::Socket(w,pawn,GetTickCount64(),socket)){Note("no-fresh-rendered-wrist");return false;}
        // Use the SAME audited native FMatrix::Rotator conversion as the
        // original socket helper. Calculate all outputs before committing.
        int angles[3]{};
        if(rot&&(!rotation||rotation(&socket,angles)!=angles))return false;
        const float before[3]{position[0],position[1],position[2]};
        for(unsigned i=0;i<3;++i){position[i]=socket.m[3][i];if(rot)rot[i]=angles[i];}
        static unsigned count=0;
        if(count++<12)Log("TMD tracked muzzle: native TMD_Muzzle owner=%08X mesh=%p frame=%u age=%llums original=(%.3f %.3f %.3f) rendered=(%.3f %.3f %.3f) [socket output; physical FX acceptance pending]",
            (unsigned)pawn,mesh,w.frame,(unsigned long long)(GetTickCount64()-w.tick),
            before[0],before[1],before[2],position[0],position[1],position[2]);
        return true;
    } __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) {
        g_tmdTrackedMuzzle=false;Note("unreadable-context-disabled");return false;
    }
}
static int __fastcall Hook(void* mesh,void*,Name name,float* position,int* rot) {
    const int result=original(mesh,name,position,rot);
    if(result)Rewrite(mesh,name,position,rot);
    return result;
}
static int __fastcall HookBone(void* mesh,void*,Name name,float* position,int* rot) {
    const int result=originalBone(mesh,name,position,rot);
    if(result)Rewrite(mesh,name,position,rot);
    return result;
}
static void* __cdecl HookMesh(void* actor) {
    void* const result=originalMesh?originalMesh(actor):nullptr;
    if(result)return result;
    if(!g_tmdTrackedMuzzle||!actor)return nullptr;
    __try {
        if(!VrModeOn()||Rd(g_gamePaused)||Rd(g_inCinematic)||Rd(g_panelOpen)||
           Rd(g_vrMenuClass)||Rd(g_vrUiMenu)||Rd(g_twoHandActive)||!TmdRaised()||
           !Rd(g_tmdOnOffHand)||!Rd(g_offHandValid))return nullptr;
        uintptr_t pawn=0;const auto tmd=NativeTmdAim::LocalTmd(pawn);
        if(tmd&&(uintptr_t)actor==tmd) {
            const auto owned=NativeOwnedInventory::P(tmd+0x204);
            if(owned&&IsLive(owned)&&
               NativeTmdAim::ExactClass(NativeOwnedInventory::P(owned+OBJ_CLASS),"SkeletalMeshComponent")) {
                static unsigned count=0;
                if(count++<12)Log("TMD tracked muzzle: GetActorMesh resolved ARvTMD=%p to owned mesh=%p",actor,(void*)owned);
                return (void*)owned;
            }
        }
    } __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) {
        return nullptr;
    }
    return nullptr;
}
static void Poll() {
    if(!g_tmdTrackedMuzzle||original)return;
    static ULONGLONG retry=0;const auto now=GetTickCount64();
    if(now<retry)return;retry=now+2000;
    __try {
        uintptr_t pawn=0;if(!NativeTmdAim::LocalTmd(pawn)){Note("waiting-owned-TMD");return;}
        const auto wrapper=NativeRegistry::Find("intUSkeletalMeshComponentexecGetSocketWorldLocationAndRotation");
        if(!NativeContextCursor::Method(wrapper,257,0x3BF1257B240D565Full)){Note("wrapper-contract");return;}
        const auto helper=NativeRifleFx::Helper(wrapper,0xee,304,0x2D4ED554DDC30DA5ull);
        const auto convert=helper?NativeRifleFx::Helper(helper,0xd5,282,0xBF658FB17AAD44DAull):0;
        if(!helper||!convert){Note("native-helper-contract");return;}

        const auto wrapperBone=NativeRegistry::Find("intUSkeletalMeshComponentexecGetBoneOrSocketLocation");
        if(!NativeContextCursor::Method(wrapperBone,257,0xC52DE2DBF5B2D7BFull)){Note("wrapper-bone-contract");return;}
        const auto helperBone=NativeRifleFx::Helper(wrapperBone,0xee,361,0x6226F84F64381E00ull);
        if(!helperBone){Note("native-bone-helper-contract");return;}

        const auto image=(uintptr_t)GetModuleHandleA(nullptr);
        const auto helperMesh=image+0xC9CEE0;
        if(!NativeContextCursor::Method(helperMesh,61,0xEC7943ED3A780D59ull)){Note("native-mesh-helper-contract");return;}

        static unsigned attempts=0;if(attempts++>=2)return;
        if(MH_CreateHook((void*)helper,(void*)&Hook,(void**)&original)!=MH_OK){Note("create-failed");return;}
        if(MH_CreateHook((void*)helperBone,(void*)&HookBone,(void**)&originalBone)!=MH_OK){Note("create-bone-failed");return;}
        if(MH_CreateHook((void*)helperMesh,(void*)&HookMesh,(void**)&originalMesh)!=MH_OK){Note("create-mesh-failed");return;}

        rotation=(Rotation)convert;thread=GetCurrentThreadId();
        if(MH_EnableHook((void*)helper)!=MH_OK||
           MH_EnableHook((void*)helperBone)!=MH_OK||
           MH_EnableHook((void*)helperMesh)!=MH_OK){
            g_tmdTrackedMuzzle=false;Note("enable-failed-disabled");return;
        }
        Log("TMD tracked muzzle: audited native socket query, bone/socket query and actor mesh resolver armed; original socket TMD_Muzzle on original wrist, both-eye witness required");
    } __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) {
        g_tmdTrackedMuzzle=false;Note("installer-unreadable-disabled");
    }
}
} // namespace NativeTmdSocket
