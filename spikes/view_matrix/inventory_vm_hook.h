#pragma once
#include "inventory_pickup_policy.h"
// Audited native pickup vtable and UE3 opcode adapters, not executable detours.
// Each policy is local-player scoped; all other calls forward unchanged.
namespace InventoryVmHook {
using Exec=void (__fastcall*)(void*,void*,void*,void*);
static Exec original=nullptr;
static uintptr_t installedSlot=0;
using Pickup= int (__fastcall*)(void*,void*,void*,int,int,int);
static Pickup originalPickup=nullptr;
static uintptr_t pickupSlot=0;
inline int __fastcall PickupCall(void* self,void* edx,void* archetype,int select,int instant,int keep) {
    using namespace NativeOwnedInventory;
    bool supported=false,local=false,linked=false;
    __try {
        local=(uintptr_t)self==pawn&&pawn&&ClassHas(P(pawn+OBJ_CLASS),"RvPlayerPawnSP");
        linked=local&&manager&&P(pawn+0x3d0)==manager;
        const uintptr_t a=(uintptr_t)archetype;
        if(local&&a&&ClassHas(P(a+OBJ_CLASS),"RvWeaponShared")) {
            char n[96]{};
            NameFromIndex(*(const int*)(a+kItemWeaponName),n,sizeof(n));
            supported=inventoryretention::WeaponBit(inventoryretention::WeaponMesh(n))!=0;
        }
    } __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) {}
    const bool retain=inventorypickup::Retain(Rd(g_keepAcquiredWeapons)!=0,local,
        thread==GetCurrentThreadId(),linked,supported,Rd(g_gamePaused)!=0,
        Rd(g_inCinematic)!=0,select,instant,keep);
    static unsigned receipts=0;
    if(local&&receipts++<12)
        Log("inventory native pickup: local=%d supported=%d linked=%d thread=%d keep=%d->%d maskBefore=%X (completion still requires chain evidence)",
            local,supported,linked,thread==GetCurrentThreadId(),keep,retain?1:keep,Mask());
    const int result=originalPickup(self,edx,archetype,select,instant,retain?1:keep);
    if(retain)next=0;
    return result;
}
static void InstallPickup();
inline int RunAuditedPickup(void* pawn, void* archetype) {
    InstallPickup();
    if (!originalPickup || !pawn || !archetype) return -1;
    return PickupCall(pawn, nullptr, archetype, 0, 0, 0);
}
// Game-authored restore path uses keep-existing=1 (verify_native_pickup.py @ 0xFF7804).
inline int RunAuditedPickupRestore(void* pawn, void* archetype) {
    InstallPickup();
    if (!originalPickup || !pawn || !archetype) return -1;
    return PickupCall(pawn, nullptr, archetype, 0, 0, 1);
}
static void InstallPickup() {
    using namespace NativeOwnedInventory;
    if(pickupSlot||!Rd(g_keepAcquiredWeapons)||!pawn)return;
    static ULONGLONG retry=0;
    const ULONGLONG now=GetTickCount64();if(now<retry)return;retry=now+2000;
    __try {
        if(!ClassHas(P(pawn+OBJ_CLASS),"RvPlayerPawnSP"))return;
        const uintptr_t vt=P(pawn),slot=vt+inventorypickup::Slot,target=P(slot);
        // Independent audited class neighbour + complete native body. Refuse a
        // different executable/override/hook; never detour executable bytes.
        if(P(vt+0xb18)!=inventoryretention::HasWeaponImpl||target<g_textLo||
           target>=g_textHi||!PauseRttProbe::SafeRange(target,inventorypickup::Bytes)||
           inventorypickup::Hash((const unsigned char*)target,inventorypickup::Bytes)!=inventorypickup::Fingerprint)return;
        DWORD old=0;if(!VirtualProtect((void*)slot,4,PAGE_READWRITE,&old))return;
        originalPickup=(Pickup)target;
        const LONG before=InterlockedCompareExchange((volatile LONG*)slot,(LONG)(uintptr_t)&PickupCall,(LONG)target);
        DWORD ignored=0;const BOOL restored=VirtualProtect((void*)slot,4,old,&ignored);
        if((uintptr_t)(uint32_t)before!=target)return;
        if(!restored) {
            InterlockedCompareExchange((volatile LONG*)slot,(LONG)target,(LONG)(uintptr_t)&PickupCall);
            VirtualProtect((void*)slot,4,old,&ignored);return;
        }
        pickupSlot=slot;
        Log("inventory native pickup: adapter installed slot=%X bodyHash=%llX; keep-argument only, no shared script write (NOT runtime acceptance)",inventorypickup::Slot,inventorypickup::Fingerprint);
    } __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) {}
}
static void __fastcall VirtualCall(void* self,void* edx,void* frame,void* result) {
    NativeOwnedInventory::ObserveThrow(self,frame);
    if(NativeOwnedInventory::SkipPickupThrow(self,frame))return;
    original(self,edx,frame,result);
}
static void Install() {
    InstallPickup();
    if(installedSlot||!Rd(g_keepAcquiredWeapons)||!g_processInternal)return;
    static unsigned attempts=0;
    if(attempts++>=4)return;
    const uintptr_t body=g_processInternal;
    // Read past an existing 5-byte detour. The two original dispatch sites
    // and their frame accesses remain in the interpreter body.
    if(!PauseRttProbe::SafeRange(body+5,0x19b))return;
    const auto* code=(const unsigned char*)(body+5);
    const uintptr_t table=inventoryvm::DispatchTable(code,0x19b);
    if(!table||!PauseRttProbe::SafeRange(table,256*4)) {
        Log("inventory VM: refused unproven opcode table");return;
    }
    const uintptr_t slot=table+0x1b*4;
    const uintptr_t target=PauseRttProbe::Pointer(slot);
    if(target<g_textLo||target>=g_textHi)return;
    // Pin the audited FFrame layout independently of the learned name vote.
    const unsigned char node[]={0x8b,0x46,0x10};
    const unsigned char step[]={0x8b,0x46,0x18,0x0f,0xb6,0x10,0x8b,0x4e,0x14};
    bool haveNode=false,haveStep=false;
    for(size_t i=0;i+sizeof(step)<=0x19b;++i) {
        if(!memcmp(code+i,node,sizeof(node)))haveNode=true;
        if(!memcmp(code+i,step,sizeof(step)))haveStep=true;
    }
    if(!haveNode||!haveStep){Log("inventory VM: refused frame layout");return;}
    DWORD old=0;
    if(!VirtualProtect((void*)slot,4,PAGE_READWRITE,&old))return;
    original=(Exec)target;
    const LONG before=InterlockedCompareExchange((volatile LONG*)slot,
        (LONG)(uintptr_t)&VirtualCall,(LONG)target);
    DWORD ignored=0;const BOOL restored=VirtualProtect((void*)slot,4,old,&ignored);
    if((uintptr_t)(uint32_t)before!=target){original=nullptr;return;}
    if(!restored) {
        // Do not accept a partially restored installation. Keep original valid
        // for any adapter invocation already in flight while undoing our slot.
        InterlockedCompareExchange((volatile LONG*)slot,(LONG)target,(LONG)(uintptr_t)&VirtualCall);
        const BOOL retry=VirtualProtect((void*)slot,4,old,&ignored);
        attempts=4;
        Log("inventory VM: adapter withdrawn after protection restore failure (retry=%d)",(int)retry);
        return;
    }
    installedSlot=slot;
    Log("inventory VM: VirtualFunction adapter installed table=%p original=%p protectionRestored=%d (runtime pickup proof pending)",
        (void*)table,(void*)target,(int)restored);
}
}
