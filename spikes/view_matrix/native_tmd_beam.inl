// Native Age FX attaches an RvCurvedBeamComponent to TMD_Muzzle. Its parent
// matrix is built from CPU SpaceBases, not the independently tracked GPU wrist.
// Correct ONLY that owned child's actual attachment update; keep its native
// material/curve endpoint, animation, energy and power rules authoritative.
namespace NativeTmdBeam {
using Update=void (__thiscall*)(void*,void*,void*,const armbind::Mat*,int);
static Update original=nullptr;
static uintptr_t attachmentReturn=0;
static DWORD thread=0;
static void Note(const char* gate) {
    static const char* previous=nullptr;static unsigned count=0;
    if(gate==previous||count>=12)return;previous=gate;++count;
    Log("TMD tracked beam: gate=%s [original component update forwarded]",gate);
}
static uintptr_t Member(uintptr_t tmd,const char* name,NativeOwnedInventory::MemberCache& cache) {
    const int off=cache.Get(NativeOwnedInventory::P(tmd+OBJ_CLASS),name);
    return off>=0?NativeOwnedInventory::P(tmd+off):0;
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
        static NativeOwnedInventory::MemberCache aging,mesh;
        const auto beam=Member(tmd,"mAgingBeam",aging);
        if(!beam||beam!=(uintptr_t)component||!IsLive(beam)||
           !NativeTmdAim::ExactClass(NativeOwnedInventory::P(beam+OBJ_CLASS),"RvCurvedBeamComponent"))return false;
        const auto owned=Member(tmd,"mOwnerMesh",mesh);
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
    static ULONGLONG retry=0;const auto now=GetTickCount64();
    if(now<retry)return;retry=now+250;
    __try {
        uintptr_t pawn=0;const auto tmd=NativeTmdAim::LocalTmd(pawn);
        if(!tmd)return;
        static NativeOwnedInventory::MemberCache aging,mesh;
        const auto beam=Member(tmd,"mAgingBeam",aging),owned=Member(tmd,"mOwnerMesh",mesh);
        if(!beam||!owned||!IsLive(beam)||!IsLive(owned))return;
        if(!NativeTmdAim::ExactClass(NativeOwnedInventory::P(beam+OBJ_CLASS),"RvCurvedBeamComponent")||
           !NativeTmdAim::ExactClass(NativeOwnedInventory::P(owned+OBJ_CLASS),"SkeletalMeshComponent"))return;
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
