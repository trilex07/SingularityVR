#include "../centurion_refill_only.h"
#include "MinHook.h"
#include <cassert>
#include <cstdio>
#include <cstring>

// Executes only this test executable's OWN fake method. No game, OpenXR,
// external process, offsets, installed files or original engine instructions.
struct FakeWeapon {
    volatile int calls=0;
    int result=7;
    centurionrefill::Counts ammo{0,10,6};
    int refills=0;bool reloading=false;
    __declspec(noinline) int Attempt(){++calls;return result;}
    __declspec(noinline) int Clip(){return ammo.clip;}
    __declspec(noinline) int Reserve(){return ammo.reserve;}
    __declspec(noinline) int Capacity(){return ammo.capacity;}
    __declspec(noinline) void Refill(){++refills;const int n=(std::min)(ammo.capacity-ammo.clip,ammo.reserve);
        ammo.clip+=n;ammo.reserve-=n;}
};
using Native=int (__thiscall*)(FakeWeapon*);
static Native original=nullptr;
static centurionnative::Gate gate;
static int mode=0;
static bool refillOnly=false;
using RefillCall=void (__thiscall*)(FakeWeapon*);
static Native getClip=nullptr,getReserve=nullptr,getCapacity=nullptr;
static RefillCall refill=nullptr;
struct Direct {
    FakeWeapon* self;
    centurionrefill::Counts Read()const{return {getClip(self),getReserve(self),getCapacity(self)};}
    void Refill()const{refill(self);}
};
static int __fastcall Hook(FakeWeapon* self,void*){
    const auto d=gate.Evaluate(mode,(std::uint32_t)(uintptr_t)self,20,
        gate.current.weapon,gate.current.sampledMs);
    if(d==centurionnative::Decision::Block)return 0;
    if(refillOnly&&d==centurionnative::Decision::Permit){
        const auto snapshot=gate.permitted;Direct direct{self};
        return centurionrefill::Commit({snapshot.clip,snapshot.reserve,snapshot.capacity},direct)==
            centurionrefill::Result::Conserved?1:0;
    }
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
    auto clipMember=&FakeWeapon::Clip,reserveMember=&FakeWeapon::Reserve,capacityMember=&FakeWeapon::Capacity;
    auto refillMember=&FakeWeapon::Refill;
    static_assert(sizeof(refillMember)==sizeof(refill),"void native thiscall pointer ABI");
    std::memcpy(&getClip,&clipMember,sizeof(getClip));std::memcpy(&getReserve,&reserveMember,sizeof(getReserve));
    std::memcpy(&getCapacity,&capacityMember,sizeof(getCapacity));std::memcpy(&refill,&refillMember,sizeof(refill));
    refillOnly=true;const int before=player.calls;
    gate.Publish({true,20,(std::uint32_t)(uintptr_t)&player,1000,true,0,10,6});
    assert(gate.Arm(20,(std::uint32_t)(uintptr_t)&player,1000));
    assert(invoke(&player)==1&&player.calls==before&&player.refills==1);
    assert((player.ammo==centurionrefill::Counts{6,4,6})&&!player.reloading);
    assert(invoke(&player)==0&&player.refills==1); // token consumed, no duplicate/full reload
    assert(invoke(&npc)==13&&npc.refills==0); // NPC still forwards, even with experiment ON
    refillOnly=false;
    gate.Clear();assert(invoke(&player)==7); // cancellation forwards baseline
    assert(MH_DisableHook((void*)target)==MH_OK);
    assert(MH_RemoveHook((void*)target)==MH_OK);
    assert(MH_Uninitialize()==MH_OK);
    assert(invoke(&player)==7);
    std::puts("PASS x86 /O2 OWN-process detour ABI: ECX/EAX/stack, void refill/int getters, no original on permit, NPC, one-shot, clean removal");
}
