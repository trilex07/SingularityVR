#include "../centurion_native_reload.h"
#include "MinHook.h"
#include <cassert>
#include <cstdio>
#include <cstring>

// Executes only this test executable's OWN fake method. No game, OpenXR,
// external process, offsets, installed files or original engine instructions.
struct FakeWeapon {
    volatile int calls=0;
    int result=7;
    __declspec(noinline) int Attempt(){++calls;return result;}
};
using Native=int (__thiscall*)(FakeWeapon*);
static Native original=nullptr;
static centurionnative::Gate gate;
static int mode=0;
static int __fastcall Hook(FakeWeapon* self,void*){
    const auto d=gate.Evaluate(mode,(std::uint32_t)(uintptr_t)self,20,
        gate.current.weapon,gate.current.sampledMs);
    if(d==centurionnative::Decision::Block)return 0;
    return original(self);
}
int main(){
    FakeWeapon player,npc;npc.result=13;
    auto member=&FakeWeapon::Attempt;
    static_assert(sizeof(member)==sizeof(Native),"single-inheritance test ABI must be one pointer");
    Native target=nullptr;std::memcpy(&target,&member,sizeof(target));
    Native volatile invoke=target; // prevent /O2 from bypassing the patched entry
    assert(invoke(&player)==7&&player.calls==1);
    assert(MH_Initialize()==MH_OK);
    assert(MH_CreateHook((void*)target,(void*)&Hook,(void**)&original)==MH_OK&&original);
    assert(MH_EnableHook((void*)target)==MH_OK);
    gate.Publish({true,20,(std::uint32_t)(uintptr_t)&player,1000});
    mode=1;assert(invoke(&player)==7&&player.calls==2); // ECX and complete EAX retained
    mode=2;assert(invoke(&player)==0&&player.calls==2); // no underlying side effects
    for(unsigned i=0;i<10000;++i){
        const int prior=player.calls;
        assert(invoke(&npc)==13); // same native method, different owner unchanged
        assert(gate.Arm(20,(std::uint32_t)(uintptr_t)&player,1000));
        assert(invoke(&player)==7&&player.calls==prior+1);
        assert(invoke(&player)==0&&player.calls==prior+1);
    }
    assert(npc.calls==10000);
    gate.Clear();assert(invoke(&player)==7); // cancellation forwards baseline
    assert(MH_DisableHook((void*)target)==MH_OK);
    assert(MH_RemoveHook((void*)target)==MH_OK);
    assert(MH_Uninitialize()==MH_OK);
    assert(invoke(&player)==7);
    std::puts("PASS x86 /O2 OWN-process detour ABI: ECX/EAX/stack, bool refusal, player/NPC, one-shot, clean removal");
}
