#define NOMINMAX
#include "../weapon_reload_runtime.h"
#include "../centurion_native_reload.h"
#include "../native_refill_only.h"
#include "MinHook.h"
#include <cassert>
#include <cstdio>

// Real detour, ONLY this fixture's fake inventory. No game binary code, process,
// Steam/OpenXR or installed files. Offline mapper separately proves UE3 seam.
struct FakeWeapon {
    volatile int calls=0;int clip=0,reserve=73,capacity=11;
    int refills=0;
    __declspec(noinline) int Attempt(){
        ++calls;if(clip>=capacity||reserve<=0)return 0;
        const int n=std::min(capacity-clip,reserve);clip+=n;reserve-=n;return 1;
    }
    __declspec(noinline) int Clip(){return clip;}
    __declspec(noinline) int Reserve(){return reserve;}
    __declspec(noinline) int Capacity(){return capacity;}
    __declspec(noinline) void Refill(){++refills;const int n=std::min(capacity-clip,reserve);clip+=n;reserve-=n;}
};
struct FakeValkyrie:FakeWeapon{};struct FakeVolk:FakeWeapon{};struct FakeKasimov:FakeWeapon{};
using Native=int (__thiscall*)(FakeWeapon*);
static Native original=nullptr;
static centurionnative::Gate centurion,next;
static uint32_t selected=0;static uint64_t clockMs=0;
static unsigned permits=0;
static bool refillOnly=false;
using RefillCall=void (__thiscall*)(FakeWeapon*);
static Native getClip=nullptr,getReserve=nullptr,getCapacity=nullptr;
static RefillCall refill=nullptr;
struct Direct {
    FakeWeapon* self;
    nativerefill::Counts Read()const{return {getClip(self),getReserve(self),getCapacity(self)};}
    void Refill()const{refill(self);}
};
static int __fastcall Hook(FakeWeapon* self,void*){
    const auto object=(uint32_t)(uintptr_t)self;
    auto d=centurion.Evaluate(2,object,20,selected,clockMs);
    bool owned=false;
    if(d==centurionnative::Decision::Forward){d=next.Evaluate(2,object,20,selected,clockMs);
        owned=true;if(d==centurionnative::Decision::Permit)++permits;}
    if(d==centurionnative::Decision::Block)return 0;
    if(refillOnly&&owned&&d==centurionnative::Decision::Permit){
        const auto snap=next.permitted;Direct direct{self};
        return nativerefill::Commit({snap.clip,snap.reserve,snap.capacity},direct)==nativerefill::Result::Conserved?1:0;
    }
    return original(self);
}
int main(){
    FakeValkyrie v;FakeVolk s;FakeKasimov k;FakeWeapon npc,oldCenturion;
    auto member=&FakeWeapon::Attempt;Native target=nullptr;
    static_assert(sizeof(member)==sizeof(target));std::memcpy(&target,&member,sizeof(target));
    Native volatile invoke=target;
    auto clipMember=&FakeWeapon::Clip,reserveMember=&FakeWeapon::Reserve,capacityMember=&FakeWeapon::Capacity;
    auto refillMember=&FakeWeapon::Refill;
    static_assert(sizeof(refillMember)==sizeof(refill));
    std::memcpy(&getClip,&clipMember,sizeof(getClip));std::memcpy(&getReserve,&reserveMember,sizeof(getReserve));
    std::memcpy(&getCapacity,&capacityMember,sizeof(getCapacity));std::memcpy(&refill,&refillMember,sizeof(refill));
    assert(MH_Initialize()==MH_OK);
    assert(MH_CreateHook((void*)target,(void*)&Hook,(void**)&original)==MH_OK);
    assert(MH_EnableHook((void*)target)==MH_OK);
    for(bool fast:{false,true}){
    refillOnly=fast;
    for(FakeWeapon* weapon:{static_cast<FakeWeapon*>(&v),static_cast<FakeWeapon*>(&s),static_cast<FakeWeapon*>(&k)}){
        const auto& profile=weaponreloadproto::profiles[weapon==&v?0:weapon==&s?1:2];
        for(unsigned round=0;round<10000;++round){
            selected=(uint32_t)(uintptr_t)weapon;clockMs+=1000;
            // Context switches must not accidentally borrow Centurion's permit.
            centurion.Publish({true,20,(uint32_t)(uintptr_t)&oldCenturion,clockMs});
            assert(centurion.Arm(20,(uint32_t)(uintptr_t)&oldCenturion,clockMs));
            weapon->clip=int(round%3);weapon->reserve=int(round%73)+1;
            weapon->capacity=weapon==&v?30:weapon==&s?6:5;
            next.Publish({true,20,selected,clockMs,true,weapon->clip,weapon->reserve,weapon->capacity});
            const int before=weapon->calls,beforeRefills=weapon->refills,total=weapon->clip+weapon->reserve;
            assert(invoke(weapon)==0&&weapon->calls==before); // AUTO blocked before native writes
            npc.clip=0;npc.reserve=73;assert(invoke(&npc)==1&&npc.clip==npc.capacity); // foreign object untouched
            weaponreloadproto::State state;weaponreloadproto::Frame frame;weaponreloadproto::Calibration cfg;
            cfg.ready=true;cfg.revision=1;frame.enabled=frame.liveIdentityVerified=true;frame.kind=profile.kind;
            frame.meshWedges=profile.meshWedges;frame.objectName=profile.objectName;
            auto& in=frame.gesture;in.context=in.supported=in.tracked=in.ammoFresh=true;
            frame.nativePermitRequired=true;const unsigned startPermits=permits;
            in.pawn=20;in.weapon=selected;in.clip=weapon->clip;in.reserve=weapon->reserve;in.capacity=weapon->capacity;
            in.socket={.2f,1,-.4f};in.pouch={-.22f,1,.18f};in.hand=in.socket;in.nowMs=clockMs;
            auto update=[&](){in.nowMs=++clockMs;return weaponreloadproto::Update(state,frame,cfg);};
            assert(update().event==weaponreloadproto::Event::None);
            frame.magazineReleaseDown=true;assert(update().ejectMagazine);
            frame.magazineReleaseDown=false;in.grip=true;in.hand=in.pouch;assert(update().reservePhysicalLeft);
            in.grip=false;in.hand=in.socket;assert(update().reloadIntent);
            assert(next.Arm(20,selected,clockMs));
            assert(invoke(weapon)==1&&weapon->calls==before+(fast?0:1)&&weapon->refills==beforeRefills+(fast?1:0));
            assert(invoke(weapon)==0&&weapon->calls==before+(fast?0:1)); // no duplicate/full native reload
            in.clip=weapon->clip;in.reserve=weapon->reserve;
            frame.nativePermitConsumed=permits==startPermits+1;
            assert(update().event==weaponreloadproto::Event::Acknowledged&&in.clip+in.reserve==total);
            next.Publish({true,20,selected,clockMs,false,in.clip,in.reserve,in.capacity});
            assert(!next.Arm(20,selected,clockMs));
            assert(invoke(weapon)==0&&weapon->calls==before+(fast?0:1)); // block-only holster context
            next.Clear();assert(invoke(weapon)==0&&weapon->calls==before+(fast?1:2)); // native full/no-reserve path when unknown
        }
    }
    }
    assert(npc.calls==60000);
    assert(MH_DisableHook((void*)target)==MH_OK&&MH_RemoveHook((void*)target)==MH_OK&&MH_Uninitialize()==MH_OK);
    puts("PASS OWN x86 /O2 shared detour: 3 weapons, 60000 OFF/ON Circle/insert ACKs, partial reserves, void/int ABI, no full reload on fast permit, no auto/duplicate/holster/Centurion leak");
}
