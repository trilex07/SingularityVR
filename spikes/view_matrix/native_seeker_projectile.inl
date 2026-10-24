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
static thread_local seekerprojectile::Launch active;

static bool Local(uintptr_t self){
    if(!g_seekerProjectileAim||!VrModeOn()||!InGameplay()||Rd(g_inCinematic)||Rd(g_gamePaused)||
       Rd(g_weaponHolstered)||!seekergeometry::Vertices(Rd(g_gunVerts))||!Rd(g_handDevValid)||!Rd(g_camPosValid)||
       self!=(uintptr_t)(uint32_t)Rd(g_weaponObj)||!IsLive(self))return false;
    seekerslot::Snapshot s;
    if(!NativeSeekerSlot::Read(s)||s.pawn!=g_playerPawn||s.current!=self)return false;
    for(unsigned i=0;i<s.count;++i)if(s.items[i].object==self&&s.items[i].seeker)return true;
    return false;
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
static void* __fastcall SpawnHook(void* self,void*,void* archetype,const V* origin,const V* direction){
    if(active.valid&&active.weapon==(uintptr_t)self&&(uintptr_t)_ReturnAddress()==spawnCaller&&notes++<12&&origin&&direction)
        Log("Seeker launch: native archetype=%p origin=(%.3f %.3f %.3f) direction=(%.5f %.5f %.5f) barrel=(%.5f %.5f %.5f); native autoaim/guidance retained",
            archetype,origin->x,origin->y,origin->z,direction->x,direction->y,direction->z,active.axis.x,active.axis.y,active.axis.z);
    return spawnOriginal(self,archetype,origin,direction);
}
static void Tick(){
    if(ready||!g_seekerProjectileAim||!NativeScopePip::spreadOriginal)return;
    static ULONGLONG next=0;const auto now=GetTickCount64();if(now<next)return;next=now+2000;
    __try {
        using NativeOwnedInventory::P;
        const auto weapon=(uintptr_t)(uint32_t)Rd(g_weaponObj);
        if(!Local(weapon))return;
        const auto vt=P(weapon),fire=P(vt+0x504),start=P(vt+0x440),aim=P(vt+0x430),spawn=P(vt+0x598);
        const auto code=&NativeRifleFx::Code;
        if(!code(fire,26,0x5B3CC29BF4ACD7E6ull)||!code(start,1110,0x0A70C4D3EEE25B1Cull)||
           !code(aim,213,0x538700ED62A99B3Eull)||!code(spawn,294,0x3DC3105D1A3C1A2Bull))return;
        const auto projectile=NativeRifleFx::Helper(fire,3,558,0xB4AEC94DBDB7CE21ull);
        if(!projectile)return;
        static unsigned attempts=0;if(attempts++>=2)return;
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
            fireOriginal=nullptr;startOriginal=nullptr;spawnOriginal=nullptr;return;
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
