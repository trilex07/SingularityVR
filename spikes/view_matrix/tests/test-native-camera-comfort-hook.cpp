#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <cstdio>
#include "../../../third_party/minhook/include/MinHook.h"
static volatile LONG g_nativeCameraComfort=0,g_vrMenuClass=0;
static uintptr_t g_camera=0,g_playerPawn=0;
static int g_offBaseEyeHeight=0x2c4;
static const int ACTOR_LOCATION=0x54,OBJ_CLASS=0x34,CAM_POV_LOC=0x420;
namespace vrscene {enum class Mode { GameplayStereo,Cinematic };}
static struct {vrscene::Mode mode=vrscene::Mode::GameplayStereo;vrscene::Mode Current(){return mode;}} g_sceneMode;
static bool vr=true,cinematic=false;
static bool VrModeOn(){return vr;}
static bool CutsceneNativeCameraArmed(){return cinematic;}
static LONG Rd(volatile LONG& v){return InterlockedCompareExchange(&v,0,0);}
static bool Readable(const void* p,size_t n) {
    MEMORY_BASIC_INFORMATION m{};
    return p && VirtualQuery(p,&m,sizeof(m)) && m.State==MEM_COMMIT &&
        !(m.Protect&(PAGE_GUARD|PAGE_NOACCESS)) &&
        (uintptr_t)p+n<=(uintptr_t)m.BaseAddress+m.RegionSize;
}
static bool IsLive(uintptr_t){return false;}
static bool NameOf(uintptr_t,char*,size_t){return false;}
static int PropOffsetInChain(uintptr_t,const char*){return -1;}
static void Log(const char*,...){}
MH_STATUS WINAPI MH_CreateHook(LPVOID,LPVOID,LPVOID*){assert(false);return MH_ERROR_UNSUPPORTED_FUNCTION;}
MH_STATUS WINAPI MH_EnableHook(LPVOID){assert(false);return MH_ERROR_UNSUPPORTED_FUNCTION;}
MH_STATUS WINAPI MH_RemoveHook(LPVOID){assert(false);return MH_ERROR_UNSUPPORTED_FUNCTION;}
#include "../native_camera_comfort.h"
static unsigned nativeCalls=0;
static void __fastcall Fake(void* self,void*,void* target,float dt) {
    assert(dt==.01f);++nativeCalls;
    auto* anchor=(cameracomfort::Pov*)((unsigned char*)self+CAM_POV_LOC);
    *anchor={{100,200,104},{1000,2000,0},95};
    auto* out=(cameracomfort::Pov*)((unsigned char*)target+8);
    *out={{120,180,108},{1100,2100,30},110};
}
int main() {
    alignas(16) unsigned char cam[0x700]{},pawn[0x400]{};
    unsigned char* target=cam+0x348;
    g_camera=(uintptr_t)cam;g_playerPawn=(uintptr_t)pawn;
    *(uintptr_t*)target=g_playerPawn;cam[0x58c]=1;
    const float position[]={100,200,30};memcpy(pawn+ACTOR_LOCATION,position,12);
    *(float*)(pawn+g_offBaseEyeHeight)=70;
    NativeCameraComfort::original=reinterpret_cast<NativeCameraComfort::Fn>(&Fake);
    auto* out=(cameracomfort::Pov*)(target+8);
    auto* anchor=(cameracomfort::Pov*)(cam+CAM_POV_LOC);
    auto run=[&](){unsigned before=nativeCalls;NativeCameraComfort::Update(cam,nullptr,target,.01f);assert(nativeCalls==before+1);};
    run();assert(out->location[0]==120 && anchor->location[2]==104); // OFF is passthrough
    g_nativeCameraComfort=1;run();
    assert(out->location[0]==100 && out->location[2]==100 && anchor->location[2]==100);
    assert(out->rotation[0]==1000 && out->fov==110 && anchor->fov==95);
    g_vrMenuClass=1;run();assert(out->location[0]==120);g_vrMenuClass=0;
    cinematic=true;run();assert(out->location[0]==120);cinematic=false;
    cam[0x58c]=3;run();assert(out->location[0]==120);cam[0x58c]=1;
    *(uintptr_t*)target=0;run();assert(out->location[0]==120);*(uintptr_t*)target=g_playerPawn;
    *(float*)(cam+0x528)=.5f;run();assert(out->location[0]==120);*(float*)(cam+0x528)=0;
    *(unsigned*)(cam+0x3f8)=2;run();assert(out->location[0]==120);*(unsigned*)(cam+0x3f8)=0;
    *(int*)(cam+0x4b0)=1;run();assert(out->location[0]==120);*(int*)(cam+0x4b0)=0;
    *(uintptr_t*)(cam+0x374)=g_playerPawn;run();assert(out->location[0]==120);*(uintptr_t*)(cam+0x374)=0;
    run();assert(out->location[2]==100);
    assert(!NativeCameraComfort::faults);
    std::puts("PASS native camera hook ABI/mock: original exactly once, output+anchor, OFF/menu/cinematic/death/target/blend/override isolation");
}
