// Native Age FX attaches an RvCurvedBeamComponent to TMD_Muzzle. Its parent
// matrix is built from CPU SpaceBases, not the independently tracked GPU wrist.
// Correct ONLY that owned child's actual attachment update; keep its native
// material/curve endpoint, animation, energy and power rules authoritative.
namespace NativeTmdBeam {
using Update=void (__thiscall*)(void*,void*,void*,const armbind::Mat*,int);
static Update original=nullptr;
static uintptr_t attachmentReturn=0;
static DWORD thread=0;
// RvTMDPower_Age::CreateFX (0xFCFEB0): ESI is the RvTMD argument at [EBP+8].
// The original stores the created beam at [ESI+530h] and attaches it to the
// mesh loaded from [ESI+204h]. These are audited native offsets, unlike the
// reflected property lookup which returned null for both in the PSVR2 census.
static constexpr uintptr_t kAgingBeam=0x530,kOwnerMesh=0x204;
static void Note(const char* gate) {
    static const char* previous=nullptr;static unsigned count=0;
    if(gate==previous||count>=12)return;previous=gate;++count;
    Log("TMD tracked beam: gate=%s [original component update forwarded]",gate);
}
static uintptr_t Beam(uintptr_t tmd){return NativeOwnedInventory::P(tmd+kAgingBeam);}
static uintptr_t OwnerMesh(uintptr_t tmd){return NativeOwnedInventory::P(tmd+kOwnerMesh);}
static bool OwnedBeam(uintptr_t tmd,uintptr_t beam) {
    // RvTMDPower_Age::CreateFX writes this exact transient component to the
    // live TMD's +530h. Unlike the owner mesh, a transient child need not have
    // an Outer chain ending at PersistentLevel; IsLive rejects the real beam.
    // The class and vtable method fingerprints are checked by Poll below.
    return tmd&&beam&&beam!=tmd&&Beam(tmd)==beam&&
        PauseRttProbe::SafeRange(beam,OBJ_CLASS+sizeof(uintptr_t))&&
        NativeTmdAim::ExactClass(NativeOwnedInventory::P(beam+OBJ_CLASS),"RvCurvedBeamComponent");
}
static bool Prepare(void* component,uintptr_t caller,const armbind::Mat* parent,
                    armbind::Mat& corrected) {
    if(!g_tmdTrackedBeam||GetCurrentThreadId()!=thread||!attachmentReturn||caller!=attachmentReturn)return false;
    __try {
        if(!VrModeOn()||Rd(g_gamePaused)||Rd(g_inCinematic)||Rd(g_panelOpen)||
           Rd(g_vrMenuClass)||Rd(g_vrUiMenu)||Rd(g_twoHandActive)||!TmdRaised()||
           !Rd(g_tmdOnOffHand)||!Rd(g_offHandValid))return false;
        uintptr_t pawn=0;const auto tmd=NativeTmdAim::LocalTmd(pawn);
        if(!tmd)return false;
        const auto beam=Beam(tmd);
        if(beam!=(uintptr_t)component||!OwnedBeam(tmd,beam))return false;
        const auto owned=OwnerMesh(tmd);
        if(!owned||!IsLive(owned)||NativeOwnedInventory::P(NativeOwnedInventory::P(owned)+0x138)+0x161!=caller) {
            Note("attachment-owner");return false;
        }
        if(!PauseRttProbe::SafeRange((uintptr_t)parent,sizeof(*parent))||!tmdmuzzle::Affine(*parent)) {
            Note("invalid-native-parent");return false;
        }
        tmdmuzzle::Witness w;
        if(!TryAcquireSRWLockShared(&NativeTmdSocket::lock))return false;
        w=NativeTmdSocket::published;ReleaseSRWLockShared(&NativeTmdSocket::lock);
        if(!tmdmuzzle::Socket(w,pawn,GetTickCount64(),corrected)) {
            Note("no-fresh-rendered-wrist");return false;
        }
        static unsigned count=0;
        if(count++<12)Log("TMD tracked beam: owned aging attachment component=%p pawn=%08X frame=%u age=%llums nativeParent=(%.3f %.3f %.3f) trackedParent=(%.3f %.3f %.3f) [native curve endpoint unchanged; physical acceptance pending]",
            component,(unsigned)pawn,w.frame,(unsigned long long)(GetTickCount64()-w.tick),
            parent->m[3][0],parent->m[3][1],parent->m[3][2],
            corrected.m[3][0],corrected.m[3][1],corrected.m[3][2]);
        return true;
    } __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) {
        g_tmdTrackedBeam=false;Note("unreadable-context-disabled");return false;
    }
}
static void Forward(void* component,uintptr_t caller,void* scene,void* owner,
                    const armbind::Mat* parent,int collision) {
    // Original SetParentToWorld uses MOVAPS and synchronously copies all 64
    // bytes. The aligned local must stay alive through the one native call.
    __declspec(align(16)) armbind::Mat corrected;
    const auto selected=Prepare(component,caller,parent,corrected)?&corrected:parent;
    original(component,scene,owner,selected,collision);
}
static void __fastcall Hook(void* component,void*,void* scene,void* owner,
                           const armbind::Mat* parent,int collision) {
    Forward(component,(uintptr_t)_ReturnAddress(),scene,owner,parent,collision);
}
static void Poll() {
    if(!g_tmdTrackedBeam||original)return;
    static ULONGLONG retry=0;static unsigned agePolls=0,ageEdges=0;
    static bool wasAge=false;
    const auto now=GetTickCount64();
    const bool age=g_tmdBeamAgeProbe&&Rd(g_rawAgePressed)!=0;
    if(age&&!wasAge&&ageEdges<4){
        ++ageEdges;retry=0;
        Log("TMD tracked beam: Square edge=%u; immediate owned-component census",ageEdges);
    }
    wasAge=age;
    if(now<retry)return;
    // A transient owned component can exist only during the Square action.
    // Keep the normal 250ms census; permit at most 60 faster reads while held.
    const bool fast=age&&agePolls<60;
    retry=now+(fast?16:250);
    if(fast)++agePolls;
    __try {
        uintptr_t pawn=0;const auto tmd=NativeTmdAim::LocalTmd(pawn);
        if(!tmd){Note("installer-no-owned-tmd");return;}
        const auto beam=Beam(tmd),owned=OwnerMesh(tmd);
        if(fast&&agePolls<=12)
            Log("TMD tracked beam: Square census sample=%u tmd=%p beam=%p owner=%p beamLive=%d ownerLive=%d",
                agePolls,(void*)tmd,(void*)beam,(void*)owned,
                beam&&IsLive(beam)?1:0,owned&&IsLive(owned)?1:0);
        if(!OwnedBeam(tmd,beam)){Note("installer-no-owned-aging-beam");return;}
        if(!owned||!IsLive(owned)){Note("installer-no-live-owner-mesh");return;}
        if(!NativeTmdAim::ExactClass(NativeOwnedInventory::P(owned+OBJ_CLASS),"SkeletalMeshComponent")){
            Note("installer-component-class");return;
        }
        const auto vt=NativeOwnedInventory::P(beam);
        const auto update=NativeOwnedInventory::P(vt+0x154),copy=NativeOwnedInventory::P(vt+0x120);
        const auto children=NativeOwnedInventory::P(NativeOwnedInventory::P(owned)+0x138);
        if(!NativeContextCursor::Method(update,124,0x81209A2B80795CAEull)||
           !NativeContextCursor::Method(copy,68,0x0DBA87D91A4465CCull)||
           !NativeContextCursor::Method(children,396,0x750AA01ABF6A6D77ull)) {
            Note("native-attachment-contract");return;
        }
        static unsigned attempts=0;if(attempts++>=2)return;
        if(MH_CreateHook((void*)update,(void*)&Hook,(void**)&original)!=MH_OK) {
            Note("create-failed");return;
        }
        attachmentReturn=children+0x161;thread=GetCurrentThreadId();
        if(MH_EnableHook((void*)update)!=MH_OK) {
            g_tmdTrackedBeam=false;Note("enable-failed-disabled");return;
        }
        Log("TMD tracked beam: audited owned aging child update armed; CPU socket parent -> final both-eye rendered wrist; native target unchanged");
    } __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) {
        g_tmdTrackedBeam=false;Note("installer-unreadable-disabled");
    }
}
} // namespace NativeTmdBeam
