#define NOMINMAX
#include <windows.h>
#include <cassert>
#include <cstdio>
#include "../seeker_slot_policy.h"
#include "../seeker_projectile_policy.h"
using V=hipknife::V;
static int g_seekerProjectileAim=1;
static volatile LONG g_inCinematic=0,g_gamePaused=0,g_weaponHolstered=0,g_gunVerts=12444,
    g_handDevValid=1,g_weaponObj=20,g_camPosValid=1;
static uintptr_t g_playerPawn=10;
static float g_camPos[3]{100,200,300};
static SRWLOCK g_nextVisualLock=SRWLOCK_INIT;
static seekergeometry::Witness g_seekerPose;
static LONG Rd(volatile LONG& p){return InterlockedCompareExchange(&p,0,0);}
static bool VrModeOn(){return true;}
static bool InGameplay(){return true;}
static bool IsLive(uintptr_t p){return p==10||p==20;}
static void Log(const char*,...){}
static void BuildGunC(float m[4][4]){memset(m,0,64);for(unsigned i=0;i<4;++i)m[i][i]=1;}
namespace NativeOwnedInventory {static uintptr_t P(uintptr_t){return 0;}}
namespace NativeSeekerSlot {
static bool owner=true;
static bool badRead=false;
static bool Read(seekerslot::Snapshot& s){if(badRead)RaiseException(EXCEPTION_ACCESS_VIOLATION,0,0,nullptr);
    s={};s.valid=owner;s.pawn=10;s.current=20;
    s.count=1;s.items[0]={20,1,0,true};return owner;}
}
namespace NativeScopePip {static void* spreadOriginal=nullptr;}
namespace NativeRifleFx {
static bool Code(uintptr_t,size_t,unsigned long long){return false;}
static uintptr_t Helper(uintptr_t,unsigned,size_t,unsigned long long){return 0;}
}
constexpr int MH_OK=0;
static int MH_CreateHook(void*,void*,void**){return 1;}
static int MH_EnableHook(void*){return 1;}
static int MH_DisableHook(void*){return 0;}
static int MH_RemoveHook(void*){return 0;}
static uintptr_t caller=0;
#define _ReturnAddress() ((void*)::caller)
#include "../native_seeker_projectile.inl"
#undef _ReturnAddress
static unsigned fires=0,starts=0,spawns=0;
static V seenOrigin,seenDirection;
static bool corrected=false,raiseFire=false;
static V* __fastcall Start(void* self,void*,V* out,V offset){
    assert(self==(void*)20&&offset.x==1&&offset.y==2&&offset.z==3);++starts;
    *out={7,8,9};return out;
}
static void* __fastcall Spawn(void* self,void*,void* archetype,const V* origin,const V* direction){
    assert(self==(void*)20&&archetype==(void*)55);++spawns;
    seenOrigin=*origin;seenDirection=*direction;return (void*)77;
}
static void* __fastcall Fire(void* self,void*){
    ++fires;corrected=NativeSeekerProjectile::active.valid;
    if(raiseFire)RaiseException(EXCEPTION_INT_DIVIDE_BY_ZERO,0,0,nullptr);
    V origin;caller=NativeSeekerProjectile::startCaller;
    assert(NativeSeekerProjectile::StartHook(self,nullptr,&origin,{1,2,3})==&origin);
    int rotation[3]{0,0,123};SeekerProjectileSpread(self,NativeSeekerProjectile::spreadCaller,0,0,rotation);
    assert(rotation[2]==123);
    constexpr double unit=6.283185307179586/65536.;
    V direction{float(cos(rotation[0]*unit)*cos(rotation[1]*unit)),
        float(cos(rotation[0]*unit)*sin(rotation[1]*unit)),float(sin(rotation[0]*unit))};
    assert(SeekerOwnsTrace((void*)10)==corrected);assert(!SeekerOwnsTrace((void*)99));
    caller=NativeSeekerProjectile::spawnCaller;
    return NativeSeekerProjectile::SpawnHook(self,nullptr,(void*)55,&origin,&direction);
}
static void ThrowTest(){
    raiseFire=true;bool caught=false;
    __try {NativeSeekerProjectile::FireHook((void*)20,nullptr);}
    __except(EXCEPTION_EXECUTE_HANDLER){caught=true;}
    assert(caught&&!NativeSeekerProjectile::active.valid);raiseFire=false;
}
int main(){
    using namespace NativeSeekerProjectile;
    ready=true;fireOriginal=(NativeSeekerProjectile::Fire)&::Fire;startOriginal=(NativeSeekerProjectile::Start)&::Start;spawnOriginal=(NativeSeekerProjectile::Spawn)&::Spawn;
    startCaller=100;spreadCaller=200;spawnCaller=300;
    armbind::Mat identity;BuildGunC(identity.m);
    assert(seekergeometry::Capture(identity,identity,10,20,GetTickCount64(),g_seekerPose));
    assert(FireHook((void*)20,nullptr)==(void*)77&&corrected);
    assert(!active.valid&&fires==1&&starts==1&&spawns==1);
    assert(fabs(seenOrigin.y-(200+seekergeometry::Muzzle.y))<.001f&&seenDirection.z>.99999f);
    g_gunVerts=12436;FireHook((void*)20,nullptr);assert(corrected);g_gunVerts=12444;
    auto unchanged=[](){const auto n=fires;FireHook((void*)20,nullptr);assert(!corrected&&fires==n+1);
        assert(seenOrigin.x==7&&seenDirection.x==1&&!NativeSeekerProjectile::active.valid);};
    g_seekerProjectileAim=0;unchanged();g_seekerProjectileAim=1;
    g_gamePaused=1;unchanged();g_gamePaused=0;g_inCinematic=1;unchanged();g_inCinematic=0;
    g_gunVerts=11387;unchanged();g_gunVerts=12444;
    g_weaponHolstered=1;unchanged();g_weaponHolstered=0;
    g_handDevValid=0;unchanged();g_handDevValid=1;
    g_camPosValid=0;unchanged();g_camPosValid=1;
    NativeSeekerSlot::owner=false;unchanged();NativeSeekerSlot::owner=true;
    NativeSeekerSlot::badRead=true;unchanged();NativeSeekerSlot::badRead=false;
    g_seekerPose.tick=GetTickCount64()-151;unchanged();
    g_seekerPose.tick=GetTickCount64();ThrowTest();
    assert(fires==starts+1&&starts==spawns);
    puts("PASS real Seeker projectile adapter: original fire/start/spawn once, native archetype/result preserved, launch correction scoped, stale/OFF/other weapon gates, SEH TLS cleanup");
}
