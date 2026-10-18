#pragma once
#include "native_camera_comfort_math.h"

// Included after the engine-object helpers and existing camera/controller state.
// See docs/native-camera-comfort.md for the static producer/consumer proof.
namespace NativeCameraComfort {
using Fn=void (__thiscall*)(void*,void*,float);
static Fn original=nullptr;
static void* installedTarget=nullptr;
static volatile LONG calls=0,applied=0,rejected=0,faults=0;
static volatile LONG rejectionReasons=0;
static bool disabledByFault=false;
static void __fastcall Update(void* self,void*,void* target,float dt) {
    // Always run native side effects, FOV interpolation and modifier timers once.
    original(self,target,dt);
    if(!Rd(g_nativeCameraComfort) || disabledByFault)return;
    InterlockedIncrement(&calls);
    __try {
        const uintptr_t cam=(uintptr_t)self,pawn=g_playerPawn;
        if(!cam || cam!=g_camera || !pawn || !Readable(self,0x698) ||
           !Readable(target,0x24) || g_offBaseEyeHeight<0 ||
           !Readable((void*)(pawn+ACTOR_LOCATION),12) ||
           !Readable((void*)(pawn+g_offBaseEyeHeight),4)) {
            InterlockedIncrement(&rejected);return;
        }
        cameracomfort::Gates gates{};
        gates.enabled=true;gates.vr=VrModeOn();
        gates.gameplay=g_sceneMode.Current()==vrscene::Mode::GameplayStereo;
        gates.cinematic=CutsceneNativeCameraArmed();
        gates.menu=Rd(g_vrMenuClass)!=0;
        gates.sameCamera=cam==g_camera && (uintptr_t)target==cam+0x348;
        gates.samePawn=*(uintptr_t*)target==pawn;
        gates.style=*(unsigned char*)(cam+0x58c); // corroborated by reflection at install
        gates.cameraBlend=*(float*)(cam+0x528);
        // Native external-override/locked/desire/camera-actor paths; preserve them.
        gates.overridden=(*(unsigned*)(cam+0x3f8)&(2u|8u|16u|32u|256u|512u))!=0 ||
            *(int*)(cam+0x4b0)!=0 || *(uintptr_t*)(cam+0x374)!=0;
            // mOverrides.Num and PendingViewTarget.Target: retain scripted transitions
        auto* out=(cameracomfort::Pov*)((unsigned char*)target+8);
        auto* anchor=(cameracomfort::Pov*)(cam+CAM_POV_LOC);
        const cameracomfort::Pov unshaken=*anchor;
        const float baseEye=*(float*)(pawn+g_offBaseEyeHeight);
        const cameracomfort::Pov nativeOut=*out;
        if(cameracomfort::Reduce(gates,(float*)(pawn+ACTOR_LOCATION),baseEye,unshaken,*out,*anchor)) {
            InterlockedIncrement(&applied);
            static bool witnessed=false;
            if(!witnessed) {
                witnessed=true;
                Log("native camera comfort: first corrected target=%p dt=%.4f baseEye=%.3f removed=(%.3f,%.3f,%.3f)",
                    (void*)pawn,dt,baseEye,nativeOut.location[0]-out->location[0],
                    nativeOut.location[1]-out->location[1],nativeOut.location[2]-out->location[2]);
            }
        } else {
            InterlockedIncrement(&rejected);
            const LONG why=(!gates.vr?1:0)|(!gates.gameplay?2:0)|(gates.cinematic?4:0)|
                (gates.menu?8:0)|(!gates.samePawn||!gates.sameCamera?16:0)|(gates.style!=1?32:0)|
                (gates.cameraBlend!=0?64:0)|(gates.overridden?128:0);
            InterlockedOr(&rejectionReasons,why?why:256);
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        disabledByFault=true;InterlockedIncrement(&faults);
    }
}
static void Poll() {
    if(!Rd(g_nativeCameraComfort))return;
    static DWORD last=0;const DWORD now=GetTickCount();
    if(last && DWORD(now-last)<1000)return;last=now;
    if(original) {
        const LONG n=InterlockedExchange(&calls,0),a=InterlockedExchange(&applied,0);
        const LONG r=InterlockedExchange(&rejected,0),f=InterlockedExchange(&faults,0);
        const LONG reasons=InterlockedExchange(&rejectionReasons,0);
        if(n||f)Log("NATIVE_CAMERA_COMFORT calls=%ld applied=%ld rejected=%ld faults=%ld reasons=%03lX",n,a,r,f,reasons);
        return;
    }
    // Existing object discovery owns identity. No GObjects walk on this path.
    const uintptr_t cam=g_camera;
    if(!cam || !Readable((void*)cam,0x698) || !IsLive(cam))return;
    const uintptr_t klass=*(uintptr_t*)(cam+OBJ_CLASS);
    char name[64]{};
    if(!NameOf(klass,name,sizeof(name)) || strcmp(name,"RvPlayerCamera"))return;
    if(PropOffsetInChain(klass,"mCurrentPOV")!=0x420 ||
       PropOffsetInChain(klass,"mCurrentShakeOffset")!=0x68c ||
       PropOffsetInChain(klass,"mCameraStyle")!=0x58c ||
       PropOffsetInChain(klass,"mOverrides")!=0x4ac ||
       PropOffsetInChain(klass,"mCameraActorBlendWeight")!=0x528)return;
    const uintptr_t table=*(uintptr_t*)cam;
    if(!Readable((void*)(table+0x438),4))return;
    void* candidate=*(void**)(table+0x438);
    if(candidate==installedTarget)return;
    installedTarget=candidate; // Refuse a mismatched target once, not every second.
    // Entire previously captured plaintext function, including final ret8.
    // Resolve through the live class vtable; never patch a hardcoded code VA.
    if((uintptr_t)GetModuleHandleW(nullptr)!=0x400000 || !Readable(candidate,0x902))return;
    unsigned hash=2166136261u;
    for(unsigned i=0;i<0x902;++i)hash=(hash^((const unsigned char*)candidate)[i])*16777619u;
    if(hash!=0xb9632f54u) { Log("native camera comfort: fingerprint rejected %08X",hash);return; }
    const MH_STATUS created=MH_CreateHook(candidate,(void*)&Update,(void**)&original);
    const MH_STATUS enabled=created==MH_OK?MH_EnableHook(candidate):created;
    if(enabled!=MH_OK) {
        if(created==MH_OK)MH_RemoveHook(candidate);
        original=nullptr;
    }
    Log("native camera comfort: checked vtable+438 create=%d enable=%d target=%p",(int)created,(int)enabled,candidate);
}
}
