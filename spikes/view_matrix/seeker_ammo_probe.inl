#include "seeker_ammo_observer.h"
static int g_seekerNativeAmmoProbe=0;
static thread_local seekerammo::Transaction g_seekerAmmoTransaction;
static bool ReadSeekerAmmo(void* object,seekerslot::Snapshot& s,nativerefill::Counts& counts){
    if(!NativeSeekerSlot::Gameplay()||!NativeSeekerSlot::Read(s)||
       s.current!=(uintptr_t)object||s.current!=(uint32_t)Rd(g_weaponObj))return false;
    bool owned=false;for(unsigned i=0;i<s.count;++i)owned|=s.items[i].seeker&&s.items[i].object==s.current;
    if(!owned)return false;
    NativeRefillBindings bindings;nativerefill::Audit audit;
    if(!ResolveNativeRefill(object,bindings,&audit)){
        static unsigned rejected=0;if(rejected++<8)LogNativeRefillAudit("seeker-read-only",3,false,audit);
        return false;
    }
    NativeRefillCalls getters{object,bindings};counts=getters.Read(); // three audited GETTERS only
    return nativerefill::Valid(counts);
}
static void BeginSeekerNativeAmmo(void* self){
    static thread_local unsigned transactions=0;
    if(!g_seekerNativeAmmoProbe||g_nextReload[3]||transactions>=8||g_seekerAmmoTransaction.active||
       !seekergeometry::Vertices(Rd(g_gunVerts)))return;
    seekerslot::Snapshot s;nativerefill::Counts counts;
    if(ReadSeekerAmmo(self,s,counts)&&g_seekerAmmoTransaction.Begin(s.pawn,s.current,counts,GetTickCount64())){
        ++transactions;Log("Seeker native ammo: BEGIN pawn=%08X weapon=%08X clip=%d reserve=%d capacity=%d nativeAttempt=%p thread=%u (read-only; physical reload OFF)",
            s.pawn,s.current,counts.clip,counts.reserve,counts.capacity,g_origCenturionAttemptReload,GetCurrentThreadId());
    }
}
static void PollSeekerNativeAmmo(void* self){
    auto& t=g_seekerAmmoTransaction;const auto now=GetTickCount64();
    if(g_nextReload[3]){t.active=false;return;}
    if(!g_seekerNativeAmmoProbe||!t.Due(now))return;
    // Natural interpreter/native call on the initiating engine thread only.
    if((uintptr_t)self!=t.weapon&&(uintptr_t)self!=t.pawn)return;
    seekerslot::Snapshot s;nativerefill::Counts after;
    const bool valid=ReadSeekerAmmo((void*)(uintptr_t)t.weapon,s,after);
    const auto status=t.Sample(valid?s.pawn:0,valid?s.current:0,after,now);
    if(status!=seekerammo::Observation::Waiting)Log("Seeker native ammo: END status=%d accepted=%d pawn=%08X weapon=%08X before=%d/%d/%d after=%d/%d/%d elapsed=%llu (1=conserved native getter acknowledgment; NOT PSVR2 proof)",
        (int)status,t.accepted,t.pawn,t.weapon,t.before.clip,t.before.reserve,t.before.capacity,
        after.clip,after.reserve,after.capacity,(unsigned long long)(now-t.started));
}
