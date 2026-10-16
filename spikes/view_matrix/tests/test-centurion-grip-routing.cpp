#include "../centurion_reload.h"
#include "../grip_latch.h"
#include <cassert>
#include <cstdio>
using namespace centurionreload;

int main(){
    // Replay runtime order: reload -> knife -> support. The same raw squeeze
    // is observed by all recognizers; only ownership filters consumers.
    State reload;griplatch::GripLatch support;hipknife::State knife;
    Frame f;f.enabled=f.context=f.tracked=f.supported=f.ammoFresh=true;
    f.pawn=20;f.weapon=10;f.clip=2;f.reserve=25;f.capacity=6;
    f.socket={.2f,1,-.5f};f.pouch=Pouch({0,1.1f,0},0).p;f.nowMs=1000;
    hipknife::Frame k;k.enabled=k.focused=k.gameplay=k.alive=k.nativeKnife=true;
    k.pawn=20;k.head={0,1.1f,0};k.hip=hipknife::Hip(k.head,0);
    k.tracked[0]=k.tracked[1]=true;k.free[0]=true;
    auto tick=[&](bool held,V p,std::uint64_t dt=20){
        f.nowMs+=dt;f.grip=held;f.hand=p;f.supportLatched=support.Latched();
        f.handBusy=hipknife::Reserves(knife,0);
        f.knifeNear=Near(p,k.hip.p,hipknife::Config{}.radius);
        const auto r=Update(reload,f);
        if(r.event==Event::GrabOld||r.event==Event::GrabFresh)support.Reset();
        k.nowMs=f.nowMs;k.grip[0]=held;k.hand[0].p=p;k.free[0]=!r.reserveHand;
        if(r.reserveHand)knife.edgeMs[0]=0;
        hipknife::Update(knife,k);
        support.Update(held&&!r.reserveHand&&!hipknife::Reserves(knife,0),
            Near(p,f.socket,.15f)?0.f:100.f,true,f.weapon,f.nowMs);
        return r;
    };
    tick(false,f.socket);
    // Press 10cm from the authored socket: support acquires before the hand
    // reaches the 7.5cm reload target. It must NOT lock reload out forever.
    const V approach=f.socket+V{.10f,0,0};
    assert(tick(true,approach).event==Event::None&&support.Latched());
    assert(tick(true,f.socket).event==Event::GrabOld&&!support.Latched());
    assert(knife.held<0&&reload.ownsGrip);
    assert(tick(true,f.socket+V{0,0,-.2f}).event==Event::Removed);
    assert(tick(false,f.socket+V{0,0,-.2f}).event==Event::Discarded);
    // Squeeze just before reaching the shoulder works, without a knife grab.
    assert(tick(true,f.pouch+V{-.16f,0,0}).event==Event::None);
    assert(tick(true,f.pouch).event==Event::GrabFresh&&knife.held<0);
    // Cancellation while physically held cannot hand over that squeeze to
    // a knife, even after moving all the way to the hip.
    f.context=false;assert(tick(true,k.hip.p).reserveHand);
    f.context=true;assert(tick(true,k.hip.p).reserveHand&&knife.held<0);
    assert(!tick(false,k.hip.p).reserveHand);
    assert(tick(true,k.hip.p).event==Event::None&&knife.held==0);
    assert(tick(true,f.socket).event==Event::None); // a held knife wins
    tick(false,k.hip.p);

    // Expired squeeze must not steal an established support grip.
    reload={};tick(false,approach);tick(true,approach);
    assert(support.Latched());
    assert(tick(true,f.socket,700).event==Event::None&&support.Latched());
    // A fresh release/re-press enables reload; continuous support is stable.
    tick(false,f.socket);assert(tick(true,f.socket).event==Event::GrabOld);
    // A tracking gap clears the acquisition window and blocks held input.
    tick(false,f.socket);reload={};tick(false,approach);tick(true,approach);
    f.tracked=false;tick(true,approach);f.tracked=true;
    assert(tick(true,f.socket).event==Event::None);

    // Physical-left shoulder remains distinct from the existing left knife
    // and right-gun hip areas at floor-seated and standing head heights/yaws.
    for(float height:{.35f,.8f,1.1f,1.8f})for(int step=0;step<360;++step){
        const float yaw=float(step)*3.14159265f/180.f;
        const V head{.8f,height,-.3f};const auto pouch=Pouch(head,yaw);
        const auto hip=hipknife::Hip(head,yaw);
        assert(hipknife::Length(pouch.p-hip.p)>Config{}.pouchRadius+hipknife::Config{}.radius);
        const V local=hipknife::Rotate({0,-std::sin(yaw*.5f),0,std::cos(yaw*.5f)},pouch.p-head);
        assert(std::fabs(local.x+.22f)<1e-5f&&std::fabs(local.y+.12f)<1e-5f&&std::fabs(local.z-.18f)<1e-5f);
    }
    std::puts("centurion grip routing L1 PASS: support takeover/knife ownership/no handover/1440 shoulder poses");
}
