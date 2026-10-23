// Own-process x86 executable fixture: real MinHook + original fld/ret8 getter,
// real production adapter, native-style caller and live-ownership failures.
// No game executable/process, Steam, GPU or proprietary assets are loaded.
#include <windows.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <initializer_list>
#include "MinHook.h"
static LONG g_dualWeaponTmdGunScale=0,g_weaponHolstered=0,g_weaponObj=1,g_handsStatus=1,g_rawAltTrigger=255;
static LONG g_rawTrigger=0,g_vrMenuClass=0,g_gamePaused=0;
static uintptr_t g_camera=0,g_controller=0,g_playerPawn=0;
static bool dual=true,nativeHands=false,bad=false,live=true;
static unsigned char pawnBytes[0x2000]{},nodeBytes[0x100]{},meshBytes[0x100]{},otherBytes[0x100]{};
static uintptr_t pawn=(uintptr_t)pawnBytes,node=(uintptr_t)nodeBytes,mesh=(uintptr_t)meshBytes;
static uintptr_t currentPawn=pawn,fieldNode=node,fieldMesh=mesh;
static uintptr_t nativePawn=pawn;
std::uintptr_t EngineNativeLocalPlayerPawn(){return nativePawn;}
static constexpr int OBJ_CLASS=0x34;
static LONG Rd(volatile LONG& x){return InterlockedCompareExchange(&x,0,0);}
static bool DualWeaponTmdContext(){return dual;}
static bool NativeAnimatedHandsNow(){return nativeHands;}
static bool IsLive(uintptr_t p){return live&&p!=0;}
static void Log(const char*,...){}
static int PropOffsetInChain(uintptr_t,const char* n){
    if(!strcmp(n,"mGunScaleCtrl"))return 0x1024;
    if(!strcmp(n,"mInviewMesh"))return 0x500;
    if(!strcmp(n,"ControlName"))return 0x40;
    if(!strcmp(n,"BoneScale"))return 0x7c;
    return -1;
}
static bool NameFromIndex(int i,char* out,size_t size){strcpy_s(out,size,i==100?"gun_scale":"clip_scale");return true;}
static bool NameOf(uintptr_t,char* out,size_t size){strcpy_s(out,size,"RvPlayerPawnSP");return true;}
namespace PauseRttProbe{static bool SafeRange(uintptr_t p,size_t){return p!=0;}}
namespace NativeTmdAim{static bool ExactClass(uintptr_t c,const char*){return c==1000;}}
namespace NativeOwnedInventory{
static uintptr_t CameraOwnedPawn(){return currentPawn;}
static bool Named(uintptr_t c,const char* n){return c==2000&&!strcmp(n,"SkelControlSingleBone");}
static uintptr_t P(uintptr_t p){
    if(bad)RaiseException(EXCEPTION_ACCESS_VIOLATION,0,0,nullptr);
    if(p==pawn+OBJ_CLASS)return 1000;
    if(p==node+OBJ_CLASS)return 2000;
    if(p==pawn+0x1024)return fieldNode;
    if(p==pawn+0x500)return fieldMesh;
    if(p==node)return 3000;
    if(p==3000+0x140)return 0xb3e400;
    return 0;
}
}
namespace NativeContextCursor{
static bool Method(uintptr_t p,size_t,uint64_t){return p==0xb3e400||p==0xaec25d;}
}
#include "../native_dual_tmd_gun_scale.h"
using NativeGetter=float (__thiscall*)(void*,int,void*);
static __declspec(noinline) float Invoke(NativeGetter f,void* self,int bone,void* m){return f(self,bone,m);}
static float Scale(){return *(float*)(nodeBytes+0x7c);}
int main(){
    static_assert(sizeof(void*)==4,"x86 fixture required");
    auto a=(unsigned char*)VirtualAlloc((void*)0xb30000,0x10000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    auto b=(unsigned char*)VirtualAlloc((void*)0xae0000,0x10000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    assert(a==(void*)0xb30000&&b==(void*)0xae0000);
    const unsigned char getter[]={0xd9,0x41,0x7c,0xc2,8,0};
    memcpy(a+0xe400,getter,sizeof(getter));
    // ECX=this; at entry [esp+4]=bone,[esp+8]=component. Two pushes then
    // CALL EAX at AEC26A, RET8. ST(0) propagates unchanged to the caller.
    const unsigned char caller[]={0xb8,0,0xe4,0xb3,0,0xff,0x74,0x24,8,0xff,0x74,0x24,8,0xff,0xd0,0xc2,8,0};
    memcpy(b+0xc25d,caller,sizeof(caller));DWORD old=0;
    assert(VirtualProtect(a,0x10000,PAGE_EXECUTE_READ,&old));
    assert(VirtualProtect(b,0x10000,PAGE_EXECUTE_READ,&old));
    FlushInstructionCache(GetCurrentProcess(),a,0x10000);FlushInstructionCache(GetCurrentProcess(),b,0x10000);
    *(int*)(nodeBytes+0x40)=100;*(float*)(nodeBytes+0x7c)=0;
    *(float*)(otherBytes+0x7c)=0;
    auto call=(NativeGetter)(b+0xc25d),direct=(NativeGetter)(a+0xe400);
    assert(MH_Initialize()==MH_OK);
    NativeDualTmdGunScale::Poll();assert(!NativeDualTmdGunScale::target);
    g_dualWeaponTmdGunScale=1;NativeDualTmdGunScale::Poll();
    assert(NativeDualTmdGunScale::target==0xb3e400&&NativeDualTmdGunScale::control==node);
    // Runtime regression: inventory camera reflection misses a pawn while
    // the independent reciprocal local-player route still identifies it.
    NativeDualTmdGunScale::control=NativeDualTmdGunScale::localPawn=0;
    NativeDualTmdGunScale::retry=0;
    currentPawn=0;NativeDualTmdGunScale::Poll();
    assert(NativeDualTmdGunScale::localPawn==pawn&&NativeDualTmdGunScale::control==node);
    currentPawn=pawn;
    uintptr_t before=0,after=0;
    __asm mov before,esp
    for(int i=0;i<10000;++i)assert(Invoke(call,nodeBytes,52,meshBytes)==1.f);
    __asm mov after,esp
    assert(before==after&&Scale()==0); // no stack leak, no engine property writes
    assert(Invoke(direct,nodeBytes,52,meshBytes)==0); // different caller refused
    assert(Invoke(call,otherBytes,55,meshBytes)==0); // clip/other controllers untouched
    assert(Invoke(call,nodeBytes,52,otherBytes)==0); // other component
    for(int invalid:{-1,256})assert(Invoke(call,nodeBytes,invalid,meshBytes)==0);
    for(int hands:{1,2}){g_handsStatus=hands;assert(Invoke(call,nodeBytes,52,meshBytes)==1);}
    for(int l2:{0,255}){g_rawAltTrigger=l2;assert(Invoke(call,nodeBytes,52,meshBytes)==1);}
    g_dualWeaponTmdGunScale=0;assert(Invoke(call,nodeBytes,52,meshBytes)==0);g_dualWeaponTmdGunScale=1;
    dual=false;assert(Invoke(call,nodeBytes,52,meshBytes)==0);dual=true;
    nativeHands=true;assert(Invoke(call,nodeBytes,52,meshBytes)==0);nativeHands=false;
    g_weaponHolstered=1;assert(Invoke(call,nodeBytes,52,meshBytes)==0);g_weaponHolstered=0;
    g_weaponObj=0;assert(Invoke(call,nodeBytes,52,meshBytes)==0);g_weaponObj=1;
    currentPawn=0;nativePawn=0;assert(Invoke(call,nodeBytes,52,meshBytes)==0);
    currentPawn=pawn;nativePawn=pawn;
    // The shipped log found the reciprocal native owner while this older
    // camera reflection rejected it. Both paths must still agree if present.
    currentPawn=0;assert(Invoke(call,nodeBytes,52,meshBytes)==1);
    currentPawn=pawn;nativePawn=(uintptr_t)otherBytes;
    assert(Invoke(call,nodeBytes,52,meshBytes)==0);
    nativePawn=pawn;
    fieldNode=0;assert(Invoke(call,nodeBytes,52,meshBytes)==0);fieldNode=node;
    fieldMesh=0;assert(Invoke(call,nodeBytes,52,meshBytes)==0);fieldMesh=mesh;
    live=false;assert(Invoke(call,nodeBytes,52,meshBytes)==0);live=true;
    *(int*)(nodeBytes+0x40)=101;assert(Invoke(call,nodeBytes,52,meshBytes)==0);*(int*)(nodeBytes+0x40)=100;
    *(int*)(nodeBytes+0x44)=1;assert(Invoke(call,nodeBytes,52,meshBytes)==0);*(int*)(nodeBytes+0x44)=0;
    *(float*)(nodeBytes+0x7c)=0.75f;assert(Invoke(call,nodeBytes,52,meshBytes)==0.75f);*(float*)(nodeBytes+0x7c)=0;
    ++NativeDualTmdGunScale::engineThread;assert(Invoke(call,nodeBytes,52,meshBytes)==0);--NativeDualTmdGunScale::engineThread;
    // Current animation retains each bone's own pose: correcting only the
    // parent scale works for arbitrary strengths and animated child scales.
    for(float strength:{0.f,0.3f,1.f})for(float childScale:{0.8f,1.f,1.2f}){
        const float neutral=Invoke(call,nodeBytes,52,meshBytes);
        assert((1+strength*(neutral-1))*childScale==childScale);
    }
    bad=true;assert(Invoke(call,nodeBytes,52,meshBytes)==0);assert(!g_dualWeaponTmdGunScale);bad=false;
    assert(MH_Uninitialize()==MH_OK);assert(Invoke(call,nodeBytes,52,meshBytes)==0);
    VirtualFree(a,0,MEM_RELEASE);VirtualFree(b,0,MEM_RELEASE);
    puts("PASS real x86 MinHook producer: 10000 ret8/x87 calls; ownership/caller/thread/OFF/native-hands/holster/GC refusal; no pose cache, no state writes");
}
