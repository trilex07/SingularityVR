#pragma once
#include "seeker_slot_policy.h"
// Only native input thread owns UObject access. XR sends a generation-bound
// request; neither renderer nor XR retains/recreates a UObject.
namespace NativeSeekerSlot {
using Query=int (__thiscall*)(void*);
static Query originalHeavy=nullptr;
static uintptr_t heavyTarget=0,switchMethod=0;
static DWORD engineThread=0;
static seekerslot::State state;
static unsigned receipts=0;
static bool Gameplay(){return VrModeOn()&&!Rd(g_gamePaused)&&!Rd(g_inCinematic)&&
    !Rd(g_vrMenuClass)&&!Rd(g_vrUiMenu)&&!Rd(g_panelOpen);}
static bool Read(seekerslot::Snapshot& s){
    using namespace NativeOwnedInventory;
    s={};s.pawn=(uint32_t)CameraOwnedPawn();
    if(!s.pawn||!IsLive(s.pawn)||!ClassHas(P(s.pawn+OBJ_CLASS),"RvPlayerPawnSP"))return false;
    const auto pc=P(s.pawn+OBJ_CLASS);
    static MemberCache invField,weaponField;
    if(invField.Get(pc,"InvManager")!=0x3d0||weaponField.Get(pc,"Weapon")!=0x3d4)return false;
    s.current=(uint32_t)P(s.pawn+0x3d4);
    const auto manager=P(s.pawn+0x3d0),mc=P(manager+OBJ_CLASS);
    static MemberCache chainField,nextField,ownerField,nameField;
    if(!manager||!IsLive(manager)||!ClassHas(mc,"RvInventoryManager")||
       chainField.Get(mc,"InventoryChain")!=0x1e0)return false;
    uintptr_t seen[128]{};unsigned visits=0;
    for(auto it=P(manager+0x1e0);it;it=P(it+0x1e0)){
        if(visits==128||!IsLive(it))return false;
        for(unsigned i=0;i<visits;++i)if(seen[i]==it)return false;
        seen[visits++]=it;
        const auto cls=P(it+OBJ_CLASS);
        if(!ClassHas(cls,"Inventory")||nextField.Get(cls,"Inventory")!=0x1e0)return false;
        if(!ClassHas(cls,"RvWeaponShared"))continue;
        // Instigator is the authoritative owner used by native weapon helpers.
        if(ownerField.Get(cls,"Instigator")!=0x98||P(it+0x98)!=s.pawn||
           nameField.Get(cls,"mWeaponName")!=0x2e8||!PauseRttProbe::SafeRange(it+0x2e8,8))return false;
        const auto name=*(const Name*)(it+0x2e8);char text[96]{};
        if(!NameFromIndex(name.index,text,sizeof(text)))return false;
        const bool seeker=!strcmp(text,"ARSteerable")&&name.number==0;
        const bool ordinary=inventoryretention::WeaponMesh(text)!=0;
        if(seeker||ordinary)s.items[s.count++]={(uint32_t)it,name.index,name.number,seeker};
    }
    s.valid=true;return true;
}
static int __fastcall Heavy(void* self,void*){
    const uintptr_t caller=(uintptr_t)_ReturnAddress();
    if(seekerslot::RetainQuery(Rd(g_seekerBackSlot)!=0,Gameplay(),
        engineThread==GetCurrentThreadId(),true,caller,switchMethod)){
        __try {
            seekerslot::Snapshot s;
            if(Read(s))for(unsigned i=0;i<s.count;++i)if(s.items[i].seeker&&s.items[i].object==(uintptr_t)self){
                // Classification substitution ONLY at native switch's two
                // heavy-drop decisions. Flags/slot/chain/ammo stay untouched.
                if(receipts++<24)Log("Seeker back slot: retained owned item=%p at native switch heavy-drop decision",self);
                return 0;
            }
        } __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) {}
    }
    return originalHeavy(self);
}
static bool Bind(const seekerslot::Snapshot& s){
    using namespace NativeOwnedInventory;
    if(heavyTarget)return true;
    uintptr_t item=0;for(unsigned i=0;i<s.count;++i)if(s.items[i].seeker)item=s.items[i].object;
    if(!item)return false;
    static MemberCache ctlField;
    const int offset=ctlField.Get(P(s.pawn+OBJ_CLASS),"Controller");
    const auto ctl=offset>=0?P(s.pawn+offset):0;
    if(!ctl||!IsLive(ctl)||!ClassHas(P(ctl+OBJ_CLASS),"RvPlayerController"))return false;
    const auto sw=P(P(ctl)+0x6b8),heavy=P(P(item)+0x554);
    if(!NativeContextCursor::Method(sw,447,0xBD62E75D7EBD62DBull)||
       !NativeContextCursor::Method(heavy,13,0xAD9AEEB598427D84ull))return false;
    static unsigned attempts=0;if(attempts++>=2)return false;
    if(MH_CreateHook((void*)heavy,(void*)&Heavy,(void**)&originalHeavy)!=MH_OK)return false;
    switchMethod=sw;engineThread=GetCurrentThreadId();
    if(MH_EnableHook((void*)heavy)!=MH_OK){
        MH_RemoveHook((void*)heavy);originalHeavy=nullptr;switchMethod=0;
        InterlockedExchange(&g_seekerBackSlot,0);return false;
    }
    heavyTarget=heavy;
    Log("Seeker back slot: exact native switch/getter contracts ready; no grants, slot/ammo writes or save edits");
    return true;
}
static void Poll(){
    const LONG requested=InterlockedExchange(&g_seekerBackRequest,0);
    if(!Rd(g_seekerBackSlot)){InterlockedExchange(&g_seekerBackAvailable,0);state={};return;}
    static ULONGLONG next=0;const auto now=GetTickCount64();
    if(!requested&&now<next)return;next=now+100;
    __try {
        seekerslot::Snapshot s;
        const bool valid=Read(s);state.Reconcile(s);
        const bool ready=valid&&state.seeker&&Bind(s);
        InterlockedExchange(&g_seekerBackAvailable,ready?(LONG)s.pawn:0);
        if(!requested||!ready||(uint32_t)requested!=s.pawn||!Gameplay())return;
        const auto target=state.ToggleTarget(s);if(!target)return;
        using namespace NativeOwnedInventory;
        // Existing owned selector ABI: FName by value, ret8. It traverses the
        // possession chain and sets the EXISTING item, with no creation path.
        const auto selector=P(P(s.pawn)+0xb14);
        if(P(P(s.pawn)+0xb18)!=inventoryretention::HasWeaponImpl||
           !NativeContextCursor::Method(selector,168,0xCB797F59504492C3ull))return;
        for(unsigned i=0;i<s.count;++i)if(s.items[i].object==target){
            using Select=void (__thiscall*)(void*,Name);
            ((Select)selector)((void*)s.pawn,{s.items[i].name,s.items[i].number});
            if(receipts++<24)Log("Seeker back slot: %s existing item=%08X pawn=%08X [native selection requested, no ammo writes]",
                s.items[i].seeker?"draw":"store",target,s.pawn);
            NativeOwnedInventory::Refresh();return;
        }
    } __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) {
        state={};InterlockedExchange(&g_seekerBackAvailable,0);
        if(receipts++<24)Log("Seeker back slot: unreadable ownership; unavailable, no native call");
    }
}
}
