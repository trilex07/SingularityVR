// Seeker ONLY: initial projectile launch, not subsequent guided flight.
#include "seeker_projectile_policy.h"
namespace NativeSeekerProjectile {
using V=hipknife::V;
using Fire=void* (__thiscall*)(void*);
using Start=V* (__thiscall*)(void*,V*,V);
using Spawn=void* (__thiscall*)(void*,void*,const V*,const V*);
static Fire fireOriginal=nullptr;
static Start startOriginal=nullptr;
static Spawn spawnOriginal=nullptr;
static uintptr_t startCaller=0,spreadCaller=0,spawnCaller=0;
static bool ready=false;
static unsigned notes=0;
static const char* localStage="not-sampled";
static void Gate(const char* why,uintptr_t weapon){
    static unsigned n=0;static const char* last=nullptr;static uintptr_t prior=0;
    if(n>=32||(last==why&&prior==weapon))return;
    last=why;prior=weapon;++n;
    Log("Seeker launch gate: stage=%s weapon=%p pawn=%p ready=%d spread=%d",why,(void*)weapon,(void*)g_playerPawn,ready,NativeScopePip::spreadOriginal!=nullptr);
}
static thread_local seekerprojectile::Launch active;

static bool Local(uintptr_t self){
    if(!g_seekerProjectileAim||!VrModeOn()||!InGameplay()||Rd(g_inCinematic)||Rd(g_gamePaused)||
       Rd(g_weaponHolstered)||!seekergeometry::Vertices(Rd(g_gunVerts))||!Rd(g_handDevValid)||!Rd(g_camPosValid)||
       self!=(uintptr_t)(uint32_t)Rd(g_weaponObj)||!IsLive(self)){localStage="context-or-tracking";return false;}
    seekerslot::Snapshot s;
    if(!NativeSeekerSlot::Read(s)){localStage=NativeSeekerSlot::stage;return false;}
    if(s.pawn!=g_playerPawn||s.current!=self){localStage="pawn-or-current-mismatch";return false;}
    for(unsigned i=0;i<s.count;++i)if(s.items[i].object==self&&s.items[i].seeker){localStage="owned";return true;}
    localStage="not-owned-seeker";return false;
}
static seekerprojectile::Launch Prepare(void* self){
    seekerprojectile::Launch result;
    __try {
        if(ready&&Local((uintptr_t)self)){
            seekergeometry::Witness w;
            if(TryAcquireSRWLockShared(&g_nextVisualLock)){
                w=g_seekerPose;ReleaseSRWLockShared(&g_nextVisualLock);
            }
            armbind::Mat gun;BuildGunC(gun.m);
            seekerprojectile::Build(w,(uint32_t)g_playerPawn,(uint32_t)(uintptr_t)self,
                GetTickCount64(),gun,{g_camPos[0],g_camPos[1],g_camPos[2]},result);
        }
    } __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) {
        result={};
    }
    return result;
}
static void* __fastcall FireHook(void* self,void*){
    const auto saved=active;active=Prepare(self);
    if(!active.valid)Gate(!strcmp(localStage,"owned")?"muzzle-witness-or-build":localStage,(uintptr_t)self);
    __try {
        // Native ProjectileFire still selects archetype, spawns, initializes
        // velocity/guidance, triggers recoil and accounts for native ammo.
        return fireOriginal(self);
    } __finally {active=saved;}
}
static V* __fastcall StartHook(void* self,void*,V* out,V offset){
    const auto caller=(uintptr_t)_ReturnAddress();
    auto* result=startOriginal(self,out,offset);
    if(active.valid&&active.weapon==(uintptr_t)self&&caller==startCaller&&result==out&&out)
        *out=active.origin;
    return result;
}
static void FinalReceipt(void* object,const V& origin,const V& direction) {
    // The pre-Spawn argument is not proof of initialized projectile flight.
    // Inspect only the actual returned native projectile, after native init.
    __try {
        using namespace NativeOwnedInventory;
        const auto p=(uintptr_t)object;
        if(!p||!IsLive(p)||!NativeTmdAim::ExactClass(P(p+OBJ_CLASS),"Projectile")) {
            Log("Seeker projectile result: object=%p no readable native Projectile (no mutation)",object);return;
        }
        const auto cls=P(p+OBJ_CLASS);
        // Original Engine.u declares all three as StructProperty, not a
        // VectorProperty class. Actor.Velocity is field 431/458: the legacy
        // 400-field RTT budget cannot observe it. This bounded, read-only
        // diagnostic opts into the same 4096 corruption backstop as ownership.
        const int loc=PauseRttProbe::Property(cls,"Location","StructProperty",4096),
            vel=PauseRttProbe::Property(cls,"Velocity","StructProperty",4096),
            rot=PauseRttProbe::Property(cls,"Rotation","StructProperty",4096);
        V location{},velocity{};int angles[3]{};
        const bool l=loc>=0&&PauseRttProbe::SafeRange(p+loc,12),
            v=vel>=0&&PauseRttProbe::SafeRange(p+vel,12),
            r=rot>=0&&PauseRttProbe::SafeRange(p+rot,12);
        if(l)memcpy(&location,(void*)(p+loc),12);
        if(v)memcpy(&velocity,(void*)(p+vel),12);
        if(r)memcpy(angles,(void*)(p+rot),12);
        Log("Seeker projectile result: object=%p locationValid=%d velocityValid=%d rotationValid=%d location=(%.3f %.3f %.3f) velocity=(%.5f %.5f %.5f) rotation=%d/%d/%d inputOrigin=(%.3f %.3f %.3f) inputDirection=(%.5f %.5f %.5f) [post native initialization; no guidance/spread changes]",
            object,l,v,r,location.x,location.y,location.z,velocity.x,velocity.y,velocity.z,
            angles[0],angles[1],angles[2],origin.x,origin.y,origin.z,direction.x,direction.y,direction.z);
    } __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) {
        Log("Seeker projectile result: unreadable returned object; native result retained");
    }
}
static void* __fastcall SpawnHook(void* self,void*,void* archetype,const V* origin,const V* direction){
    const bool receipt=active.valid&&active.weapon==(uintptr_t)self&&
        (uintptr_t)_ReturnAddress()==spawnCaller&&notes++<12&&origin&&direction;
    if(receipt)
        Log("Seeker launch: native archetype=%p origin=(%.3f %.3f %.3f) direction=(%.5f %.5f %.5f) barrel=(%.5f %.5f %.5f) expectedOrigin=(%.3f %.3f %.3f); native autoaim/guidance retained",
            archetype,origin->x,origin->y,origin->z,direction->x,direction->y,direction->z,active.axis.x,active.axis.y,active.axis.z,active.origin.x,active.origin.y,active.origin.z);
    void* result=spawnOriginal(self,archetype,origin,direction);
    if(receipt)FinalReceipt(result,*origin,*direction);
    return result;
}
static void Tick(){
    if(ready||!g_seekerProjectileAim)return;
    if(!NativeScopePip::spreadOriginal){Gate("spread-hook-not-ready",(uintptr_t)(uint32_t)Rd(g_weaponObj));return;}
    static ULONGLONG next=0;const auto now=GetTickCount64();if(now<next)return;next=now+2000;
    __try {
        using NativeOwnedInventory::P;
        const auto weapon=(uintptr_t)(uint32_t)Rd(g_weaponObj);
        if(!Local(weapon)){Gate(localStage,weapon);return;}
        const auto vt=P(weapon),fire=P(vt+0x504),start=P(vt+0x440),aim=P(vt+0x430),spawn=P(vt+0x598);
        const auto code=&NativeRifleFx::Code;
        if(!code(fire,26,0x5B3CC29BF4ACD7E6ull)||!code(start,1110,0x0A70C4D3EEE25B1Cull)||
           !code(aim,213,0x538700ED62A99B3Eull)||!code(spawn,294,0x3DC3105D1A3C1A2Bull)){Gate("method-fingerprint",weapon);return;}
        const auto projectile=NativeRifleFx::Helper(fire,3,558,0xB4AEC94DBDB7CE21ull);
        if(!projectile){Gate("projectile-helper",weapon);return;}
        static unsigned attempts=0;if(attempts++>=2){Gate("hook-attempt-limit",weapon);return;}
        // Arm all hooks transactionally. Inactive wrappers always forward.
        uintptr_t targets[]{fire,start,spawn};void* detours[]{(void*)&FireHook,(void*)&StartHook,(void*)&SpawnHook};
        void** originals[]{(void**)&fireOriginal,(void**)&startOriginal,(void**)&spawnOriginal};
        unsigned created=0;
        for(;created<3;++created)if(MH_CreateHook((void*)targets[created],detours[created],originals[created])!=MH_OK)break;
        unsigned enabled=0;
        if(created==3)for(;enabled<3;++enabled)if(MH_EnableHook((void*)targets[enabled])!=MH_OK)break;
        if(created!=3||enabled!=3){
            for(unsigned i=0;i<enabled;++i)MH_DisableHook((void*)targets[i]);
            for(unsigned i=0;i<created;++i)MH_RemoveHook((void*)targets[i]);
            fireOriginal=nullptr;startOriginal=nullptr;spawnOriginal=nullptr;Gate("hook-transaction",weapon);return;
        }
        startCaller=projectile+0x88;spawnCaller=projectile+0x1dd;spreadCaller=aim+0xca;
        ready=true;
        Log("Seeker launch: audited RocketLauncher -> ProjectileFire -> physical origin / AddSpread -> native Spawn; opt-in ready");
    } __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) {}
}
}
void SeekerProjectileTick(){NativeSeekerProjectile::Tick();}
bool SeekerOwnsTrace(void* source){
    const auto& a=NativeSeekerProjectile::active;
    return a.valid&&((uintptr_t)source==a.pawn||(uintptr_t)source==a.weapon);
}
void SeekerProjectileSpread(void* weapon,uintptr_t caller,int pitch,int yaw,int* result){
    if(caller==NativeSeekerProjectile::spreadCaller)
        seekerprojectile::Spread(NativeSeekerProjectile::active,(uint32_t)(uintptr_t)weapon,pitch,yaw,result);
}
