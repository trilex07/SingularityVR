#include "seeker_ammo_observer.h"

// This diagnostic watches an original AttemptReload transaction. It never
// calls Reload/Refill itself or changes the request, return value or inventory.
static thread_local seekerammo::Transaction g_seekerAmmoTransaction;
static thread_local bool g_seekerAmmoInsideAttempt=false;

static bool ReadSeekerAmmo(void* object,seekerslot::Snapshot& s,nativerefill::Counts& counts){
    if(!NativeSeekerSlot::Gameplay()||Rd(g_weaponHolstered)||!NativeSeekerSlot::Read(s)||
       s.current!=(uintptr_t)object||s.current!=(uint32_t)Rd(g_weaponObj))return false;
    bool owned=false;
    for(unsigned i=0;i<s.count;++i)owned|=s.items[i].seeker&&s.items[i].object==s.current;
    if(!owned)return false;
    NativeRefillBindings bindings;nativerefill::Audit audit;
    if(!ResolveNativeRefill(object,bindings,&audit)){
        static unsigned rejected=0;
        if(rejected++<8)LogNativeRefillAudit("seeker-read-only",3,false,audit);
        return false;
    }
    NativeRefillCalls getters{object,bindings};
    counts=getters.Read(); // only the three fingerprinted getters
    return nativerefill::Valid(counts);
}
static void BeginSeekerNativeAmmo(void* self, ULONGLONG now){
    if((uintptr_t)self!=(uint32_t)Rd(g_weaponObj)||!seekergeometry::Vertices(Rd(g_gunVerts)))return;
    auto& t=g_seekerAmmoTransaction;
    if(t.active){
        Log("Seeker native ammo: superseded by another native reload attempt; transfer inconclusive");
        t.active=false;
        return;
    }
    static thread_local ULONGLONG retry=0;
    static thread_local unsigned checks=0;
    if(now<retry||checks>=16)return;
    ++checks;retry=now+250;
    seekerslot::Snapshot s;nativerefill::Counts counts{};
    if(!ReadSeekerAmmo(self,s,counts))return;
    if(t.Begin(s.pawn,s.current,counts,now))
        Log("Seeker native ammo: BEGIN pawn=%08X weapon=%08X clip=%d reserve=%d capacity=%d (read-only)",
            s.pawn,s.current,counts.clip,counts.reserve,counts.capacity);
    else if(!counts.reserve)
        Log("Seeker native ammo: no reserve (clip=%d capacity=%d); native transfer cannot be verified",
            counts.clip,counts.capacity);
}

static void SampleSeekerNativeAmmo(void* self,bool immediate){
    auto& t=g_seekerAmmoTransaction;
    if(!t.active||g_seekerAmmoInsideAttempt)return;
    if(!NativeSeekerSlot::Gameplay()||Rd(g_weaponHolstered)||
       t.pawn!=(uint32_t)g_playerPawn||t.weapon!=(uint32_t)Rd(g_weaponObj)){
        t.active=false;
        Log("Seeker native ammo: context lost; transfer inconclusive");
        return;
    }
    const auto now=GetTickCount64();
    if((!immediate&&!t.Due(now))||((uintptr_t)self!=t.weapon&&(uintptr_t)self!=t.pawn))return;
    seekerslot::Snapshot s;nativerefill::Counts after{};
    const bool valid=ReadSeekerAmmo((void*)(uintptr_t)t.weapon,s,after);
    const auto status=t.Sample(valid?s.pawn:0,valid?s.current:0,after,now);
    if(status!=seekerammo::Observation::Waiting)
        Log("Seeker native ammo: END status=%d accepted=%d pawn=%08X weapon=%08X before=%d/%d/%d after=%d/%d/%d elapsed=%llu (1=conserved; not PSVR2 proof)",
            (int)status,t.accepted,t.pawn,t.weapon,t.before.clip,t.before.reserve,t.before.capacity,
            after.clip,after.reserve,after.capacity,(unsigned long long)(now-t.started));
}

static void GuardSeekerAmmoSample(void* self,bool immediate){
    __try { SampleSeekerNativeAmmo(self,immediate); }
    __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){
        g_seekerAmmoTransaction.active=false;
        Log("Seeker native ammo: unreadable during sample; transfer inconclusive");
    }
}
static void PollSeekerNativeAmmo(void* self){
    if(g_seekerAmmoTransaction.active)GuardSeekerAmmoSample(self,false);
}
static void GuardSeekerAmmoBegin(void* self){
    __try { BeginSeekerNativeAmmo(self,GetTickCount64()); }
    __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){
        g_seekerAmmoTransaction.active=false;
        Log("Seeker native ammo: unreadable before attempt; transfer inconclusive");
    }
}

static int ObserveSeekerNativeAttempt(void* self){
    if(g_seekerAmmoInsideAttempt)return g_origCenturionAttemptReload(self);
    GuardSeekerAmmoBegin(self);
    struct AttemptScope {
        AttemptScope(){g_seekerAmmoInsideAttempt=true;}
        ~AttemptScope(){g_seekerAmmoInsideAttempt=false;}
    };
    const int result=[&]{AttemptScope scope;return g_origCenturionAttemptReload(self);}();
    auto& t=g_seekerAmmoTransaction;
    if(t.active&&(uintptr_t)self==t.weapon){
        t.accepted=result!=0;
        GuardSeekerAmmoSample(self,true);
        if(!result&&t.active){
            t.active=false;
            Log("Seeker native ammo: native attempt refused; transfer inconclusive");
        }
    }
    return result;
}
