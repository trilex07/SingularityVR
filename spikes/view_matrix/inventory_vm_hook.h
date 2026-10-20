#pragma once
// A single UE3 opcode table slot, not another executable detour. The exact
// local pickup expression is consumed by policy; all other calls forward.
namespace InventoryVmHook {
using Exec=void (__fastcall*)(void*,void*,void*,void*);
static Exec original=nullptr;
static uintptr_t installedSlot=0;
static void __fastcall VirtualCall(void* self,void* edx,void* frame,void* result) {
    NativeOwnedInventory::ObserveThrow(self,frame);
    if(NativeOwnedInventory::SkipPickupThrow(self,frame))return;
    original(self,edx,frame,result);
}
static void Install() {
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
