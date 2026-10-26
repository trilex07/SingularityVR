// Runs the real engine-thread slot adapter against owned synthetic objects.
// No game/process access, native acquisition or ammunition writer.
#define NOMINMAX
#include <windows.h>
#include <intrin.h>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cstdint>
#include <map>
volatile LONG g_seekerBackSlot=1,g_seekerBackRequest=0,g_seekerBackAvailable=0;
volatile LONG g_gamePaused=0,g_inCinematic=0,g_vrMenuClass=0,g_vrUiMenu=0,g_panelOpen=0;
constexpr unsigned OBJ_CLASS=32;
static LONG Rd(volatile LONG& p){return InterlockedCompareExchange(&p,0,0);}
static bool VrModeOn(){return true;}
static void Log(const char*,...){}
alignas(16) static unsigned char pawn[4096]{},manager[1024]{},seeker[1024]{},rifle[1024]{},controller[1024]{},unrelated[1024]{};
static uintptr_t Ptr(void* p){return (uintptr_t)p;}
static bool liveSeeker=true,wrongOwner=false,badMethod=false;
static uintptr_t currentPawn=Ptr(pawn);
static int reflectedInstigator=0xcc;
static std::map<uintptr_t,uintptr_t> fields;
static bool IsLive(uintptr_t p){return p&&(p!=Ptr(seeker)||liveSeeker);}
namespace PauseRttProbe {static bool SafeRange(uintptr_t p,size_t){return p!=0;}}
namespace inventoryretention {
constexpr uintptr_t HasWeaponImpl=0x102f700;
static unsigned WeaponMesh(const char* n){return !strcmp(n,"ARRifle")?11387:0;}
}
namespace NativeOwnedInventory {
struct Name {int index,number;};
static unsigned refreshes=0;
static void Refresh(){++refreshes;}
static uintptr_t CameraOwnedPawn(){return 0;} // recorded PR60 failure
static uintptr_t P(uintptr_t a){
    if(wrongOwner&&a==Ptr(seeker)+0xcc)return Ptr(pawn)+4;
    auto i=fields.find(a);return i==fields.end()?0:i->second;
}
static bool ClassHas(uintptr_t c,const char* n){
    if(c==1)return !strcmp(n,"RvPlayerPawnSP");
    if(c==2)return !strcmp(n,"RvInventoryManager");
    if(c==3)return !strcmp(n,"Inventory")||!strcmp(n,"RvWeaponShared");
    if(c==4)return !strcmp(n,"RvPlayerController");return false;
}
static bool NameFromIndex(int i,char* out,size_t n){
    const char* text=i==10?"ARSteerable":i==20?"ARRifle":i==30?"UnrelatedWeapon":nullptr;
    if(!text)return false;strcpy_s(out,n,text);return true;
}
#include "seeker_registered_names.h"
struct MemberCache {int Get(uintptr_t,const char* n){
    bool registered=false;for(auto name:productionNames)if(!strcmp(name,n))registered=true;
    if(!registered)return -1;
    if(!strcmp(n,"InvManager"))return 0x3d0;if(!strcmp(n,"Weapon"))return 0x3d4;
    if(!strcmp(n,"InventoryChain")||!strcmp(n,"Inventory"))return 0x1e0;
    if(!strcmp(n,"Instigator"))return reflectedInstigator;if(!strcmp(n,"mWeaponName"))return 0x2e8;
    if(!strcmp(n,"Controller"))return 0x80;return -1;
}};
}
static unsigned selected=0;
static bool selectorCompletes=true;
static void __fastcall SelectOwned(void* self,void*,NativeOwnedInventory::Name n){
    assert(self==pawn&&n.number==0);++selected;
    assert(n.index==10||n.index==20);
    if(selectorCompletes)fields[Ptr(pawn)+0x3d4]=n.index==10?Ptr(seeker):Ptr(rifle);
}
namespace NativeContextCursor {static bool Method(uintptr_t p,size_t,unsigned long long){return p&&!badMethod;}}
static int __fastcall QueryHeavy(void*,void*){return 1;}
constexpr int MH_OK=0;
static unsigned created=0,removed=0;
static bool failEnable=false;
static int MH_CreateHook(void*,void*,void** out){++created;*out=(void*)&QueryHeavy;return 0;}
static int MH_EnableHook(void*){return failEnable?1:0;}
static int MH_RemoveHook(void*){++removed;return 0;}
#include "../native_local_player.h"
static nativeplayer::Identity ReadNativeLocalPlayer(){return {Ptr(controller),currentPawn,nativeplayer::Stage::Ready};}
#include "../native_seeker_slot.h"
static void Request(){InterlockedExchange(&g_seekerBackRequest,(LONG)currentPawn);NativeSeekerSlot::Poll();}
static void Setup(){
    fields.clear();currentPawn=Ptr(pawn);liveSeeker=true;wrongOwner=false;badMethod=false;
    g_seekerBackSlot=1;g_gamePaused=g_inCinematic=0;
    fields[Ptr(pawn)+OBJ_CLASS]=1;fields[Ptr(manager)+OBJ_CLASS]=2;
    fields[Ptr(seeker)+OBJ_CLASS]=fields[Ptr(rifle)+OBJ_CLASS]=3;fields[Ptr(controller)+OBJ_CLASS]=4;
    fields[Ptr(pawn)+0x3d0]=Ptr(manager);fields[Ptr(pawn)+0x3d4]=Ptr(rifle);
    fields[Ptr(manager)+0x1e0]=Ptr(rifle);fields[Ptr(rifle)+0x1e0]=Ptr(seeker);
    fields[Ptr(seeker)+0xcc]=fields[Ptr(rifle)+0xcc]=Ptr(pawn);
    fields[Ptr(pawn)+0x80]=Ptr(controller);
    fields[Ptr(pawn)]=0x10000;fields[0x10000+0xb14]=Ptr((void*)&SelectOwned);
    fields[0x10000+0xb18]=inventoryretention::HasWeaponImpl;
    fields[Ptr(controller)]=0x20000;fields[0x20000+0x6b8]=0x1047220;
    fields[Ptr(seeker)]=0x30000;fields[0x30000+0x554]=0x110000;
    *(NativeOwnedInventory::Name*)(seeker+0x2e8)={10,0};
    *(NativeOwnedInventory::Name*)(rifle+0x2e8)={20,0};
}
static void RetentionCallsiteTest(){
    // Exercise the ACTUAL compiled /O2 Heavy callback and _ReturnAddress,
    // rather than proving only RetainQuery's arithmetic. This is our own
    // executable CPU fixture, not a game hook or game-memory modification.
    static_assert(sizeof(void*)==4,"Seeker native ABI fixture must be x86");
    auto* code=(unsigned char*)VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    assert(code);memset(code,0xcc,4096);
    auto emit=[&](unsigned ret,void* self){
        auto* p=code+ret-12;
        *p++=0xb9;auto ptr=(uint32_t)(uintptr_t)self;memcpy(p,&ptr,4);p+=4; // mov ecx,self
        *p++=0xb8;ptr=(uint32_t)(uintptr_t)&NativeSeekerSlot::Heavy;memcpy(p,&ptr,4);p+=4;
        *p++=0xff;*p++=0xd0;*p=0xc3; // call eax ; ret (actual return address)
    };
    emit(0xd2,seeker);emit(0xe9,seeker);emit(0x102,seeker);emit(0x122,rifle);
    DWORD old=0;assert(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old));
    assert(FlushInstructionCache(GetCurrentProcess(),code,4096));
    const auto saved=NativeSeekerSlot::switchMethod;NativeSeekerSlot::switchMethod=(uintptr_t)code;
    using Stub=int(__cdecl*)();
    auto call=[&](unsigned ret){return ((Stub)(code+ret-12))();};
    assert(call(0xd2)==0&&call(0xe9)==0); // both audited native switch decisions
    assert(call(0x102)==1&&call(0x122)==1); // unrelated call / ordinary weapon
    g_seekerBackSlot=0;assert(call(0xd2)==1);g_seekerBackSlot=1;
    g_gamePaused=1;assert(call(0xd2)==1);g_gamePaused=0;
    wrongOwner=true;assert(call(0xd2)==1);wrongOwner=false;
    const auto ownerThread=NativeSeekerSlot::engineThread;
    NativeSeekerSlot::engineThread=0;assert(call(0xd2)==1);NativeSeekerSlot::engineThread=ownerThread;
    NativeSeekerSlot::switchMethod=saved;assert(VirtualFree(code,0,MEM_RELEASE));
    puts("PASS emitted x86 retention callback: exact +D2/+E9 return sites, ordinary/unrelated/OFF/pause/owner/thread forward original");
}
int main(){
    Setup();seekerslot::Snapshot s;assert(NativeSeekerSlot::Read(s)&&s.count==2);
    reflectedInstigator=0x98;assert(!NativeSeekerSlot::Read(s));
    assert(!strcmp(NativeSeekerSlot::stage,"instigator-offset"));reflectedInstigator=0xcc;
    // A foreign unrelated item ahead of the real weapons does not veto them.
    fields[Ptr(unrelated)+OBJ_CLASS]=3;
    fields[Ptr(manager)+0x1e0]=Ptr(unrelated);
    fields[Ptr(unrelated)+0x1e0]=Ptr(rifle);
    fields[Ptr(unrelated)+0xcc]=Ptr(pawn)+4;
    *(NativeOwnedInventory::Name*)(unrelated+0x2e8)={30,0};
    assert(NativeSeekerSlot::Read(s)&&s.count==2);
    // Even a recognized but foreign weapon is excluded, never selected.
    *(NativeOwnedInventory::Name*)(unrelated+0x2e8)={20,0};
    assert(NativeSeekerSlot::Read(s)&&s.count==2);
    // Old build incorrectly required +98: poison it independently of +CC.
    fields[Ptr(seeker)+0x98]=fields[Ptr(rifle)+0x98]=0xDEAD0001;
    assert(NativeSeekerSlot::Read(s)&&s.count==2);
    // Capture every byte, including arbitrary synthetic native ammunition fields.
    memset(seeker+0x300,0xa7,100);memset(rifle+0x300,0x3c,100);
    unsigned char beforeSeeker[1024],beforeRifle[1024];
    memcpy(beforeSeeker,seeker,1024);memcpy(beforeRifle,rifle,1024);
    for(unsigned i=0;i<50;++i){Request();assert(fields[Ptr(pawn)+0x3d4]==Ptr(seeker));
        Request();assert(fields[Ptr(pawn)+0x3d4]==Ptr(rifle));}
    assert(selected==100&&NativeOwnedInventory::refreshes==100&&created==1);
    assert(!memcmp(beforeSeeker,seeker,1024)&&!memcmp(beforeRifle,rifle,1024));
    assert(NativeSeekerSlot::Heavy(seeker,nullptr)==1); // non-switch caller forwarded
    RetentionCallsiteTest();
    auto refusal=[&](){const auto count=selected;Request();assert(count==selected);};
    g_gamePaused=1;refusal();g_gamePaused=0;g_inCinematic=1;refusal();g_inCinematic=0;
    g_seekerBackSlot=0;refusal();assert(!g_seekerBackAvailable);g_seekerBackSlot=1;
    wrongOwner=true;refusal();assert(!g_seekerBackAvailable);wrongOwner=false;
    liveSeeker=false;refusal();assert(!g_seekerBackAvailable);liveSeeker=true;
    fields[Ptr(seeker)+0x1e0]=Ptr(rifle);refusal();assert(!g_seekerBackAvailable);fields.erase(Ptr(seeker)+0x1e0);
    fields[Ptr(rifle)+0x1e0]=0;refusal();fields[Ptr(rifle)+0x1e0]=Ptr(seeker);
    badMethod=true;refusal();badMethod=false;
    g_seekerBackRequest=(LONG)Ptr(pawn);currentPawn=Ptr(pawn)+4;
    NativeSeekerSlot::Poll();assert(selected==100&&!g_seekerBackAvailable);currentPawn=Ptr(pawn);
    // A selector returning is not proof of equip. Keep pending until observed.
    selectorCompletes=false;Request();assert(NativeSeekerSlot::pendingTarget==Ptr(seeker));
    NativeSeekerSlot::pendingUntil=0;NativeSeekerSlot::Poll();
    // Poll is normally throttled; force a fresh engine sample without acquisition.
    Sleep(110);NativeSeekerSlot::Poll();assert(!NativeSeekerSlot::pendingTarget);
    assert(fields[Ptr(pawn)+0x3d4]==Ptr(rifle));selectorCompletes=true;
    Request();assert(NativeSeekerSlot::pendingTarget==Ptr(seeker));
    Sleep(110);NativeSeekerSlot::Poll();assert(!NativeSeekerSlot::pendingTarget);
    assert(NativeSeekerSlot::Read(s));NativeSeekerSlot::heavyTarget=0;failEnable=true;
    assert(!NativeSeekerSlot::Bind(s)&&!g_seekerBackSlot&&removed==1&&!NativeSeekerSlot::originalHeavy);
    puts("PASS real Seeker slot adapter: 100 native-owned selector ABI calls; byte-exact weapon/ammo conservation; pause/cinema/OFF/owner/cycle/deletion/pawn/method gates; failed hook cleanup");
}
