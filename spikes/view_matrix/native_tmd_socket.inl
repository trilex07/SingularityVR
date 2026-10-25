// Exact native socket query only: no particle recreation, power/energy writes,
// camera changes or global weapon trace edits. TMD_Muzzle remains native-owned.
namespace NativeTmdSocket {
using Name=NativeOwnedInventory::Name;
using Query=int (__thiscall*)(void*,Name,float*,int*);
using Rotation=int* (__thiscall*)(const void*,int*);
static Query original=nullptr;
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
        static NativeOwnedInventory::MemberCache ownerMesh;
        const int off=tmd?ownerMesh.Get(NativeOwnedInventory::P(tmd+OBJ_CLASS),"mOwnerMesh"):-1;
        const auto owned=off>=0?NativeOwnedInventory::P(tmd+off):0;
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
        static unsigned attempts=0;if(attempts++>=2)return;
        if(MH_CreateHook((void*)helper,(void*)&Hook,(void**)&original)!=MH_OK){Note("create-failed");return;}
        rotation=(Rotation)convert;thread=GetCurrentThreadId();
        if(MH_EnableHook((void*)helper)!=MH_OK){
            g_tmdTrackedMuzzle=false;Note("enable-failed-disabled");return;
        }
        Log("TMD tracked muzzle: audited native socket query armed; original socket TMD_Muzzle on original wrist, both-eye witness required");
    } __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) {
        g_tmdTrackedMuzzle=false;Note("installer-unreadable-disabled");
    }
}
} // namespace NativeTmdSocket
