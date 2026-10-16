#include "../centurion_reload.h"
#include <cassert>
#include <limits>
#include <cstdio>
using namespace centurionreload;
int main(){
    State s;Frame f;f.enabled=f.context=f.tracked=f.supported=true;
    f.weapon=10;f.pawn=20;f.clip=2;f.reserve=25;f.capacity=6;f.ammoFresh=true;
    f.socket={.2f,1,-.5f};f.pouch={0,.6f,-.34f};f.hand=f.socket;
    auto tick=[&](bool grip,V p){f.nowMs+=20;f.grip=grip;f.hand=p;return Update(s,f);};
    tick(false,f.socket);
    assert(tick(true,f.socket).event==Event::GrabOld);
    assert(tick(true,{.2f,1,-.7f}).event==Event::Removed);
    assert(tick(false,{.2f,1,-.7f}).event==Event::Discarded);
    assert(tick(true,f.pouch).event==Event::GrabFresh);
    assert(tick(true,f.socket).reserveHand);
    assert(tick(false,f.socket).requestNative);
    for(int i=0;i<10;++i)assert(!tick(false,f.socket).requestNative);
    f.clip=6;f.reserve=21;assert(tick(false,f.socket).event==Event::Acknowledged);
    // Full gun, no reserve, tracking, knife and long-held support refuse entry.
    for(int gate=0;gate<10;++gate){
        s={};f.clip=2;f.reserve=25;f.capacity=6;f.grip=false;f.hand=f.socket;Update(s,f);
        f.grip=true;f.nowMs+=20;
        if(gate==0)f.clip=6;if(gate==1)f.reserve=0;if(gate==2)f.tracked=false;
        if(gate==3)f.knifeNear=true;
        if(gate==4){f.supportLatched=true;s.priorGrip=true;}
        if(gate==5)f.supported=false;if(gate==6)f.firing=true;
        if(gate==7)f.handBusy=true;
        if(gate==8)f.ammoFresh=false;if(gate==9)f.context=false;
        assert(Update(s,f).event==Event::None&&s.phase==Phase::Idle);
        f.tracked=f.supported=true;f.knifeNear=f.supportLatched=f.firing=f.handBusy=false;
        f.ammoFresh=f.context=true;
    }
    // Cancelled held squeeze never leaks into TMD; must release/re-grip.
    s={};f.clip=2;f.reserve=25;tick(false,f.socket);tick(true,f.socket);
    f.context=false;assert(tick(true,f.socket).reserveHand);
    f.context=true;assert(tick(true,f.socket).event==Event::None);
    assert(!tick(false,f.socket).reserveHand);assert(tick(true,f.socket).event==Event::GrabOld);
    f.weapon++;assert(tick(true,f.socket).event==Event::Cancelled);
    assert(tick(true,f.socket).event==Event::None);
    tick(false,f.socket);tick(true,f.socket);f.hand.x=std::numeric_limits<float>::quiet_NaN();
    assert(Update(s,f).event==Event::Cancelled);
    // Changing ammo while handling the old clip cancels, never writes ammo.
    s={};f.hand=f.socket;tick(false,f.socket);tick(true,f.socket);f.clip++;
    assert(tick(true,f.socket).event==Event::Cancelled);
    // A pickup alone is not a reload acknowledgement.
    s={};s.phase=Phase::WaitingNative;s.startClip=2;s.startReserve=25;s.startCapacity=6;s.since=f.nowMs;
    f.clip=6;f.reserve=25;assert(tick(false,f.socket).event==Event::None);
    f.nowMs+=4000;assert(Update(s,f).event==Event::Rejected);
    NativeTap tap;assert(!tap.Update(true,true,100));assert(!tap.Update(true,false,110));
    assert(tap.Update(true,false,120));assert(!tap.Update(false,false,130));
    assert(!tap.Update(true,false,140));assert(!tap.Update(true,false,1000));
    // Belt overlap is arbitrated in favour of the existing left knife.
    s={};s.phase=Phase::NeedAmmo;s.startClip=f.clip;s.startReserve=f.reserve;s.startCapacity=6;
    s.since=f.nowMs;f.grip=false;f.hand=f.pouch;Update(s,f);
    f.knifeNear=true;assert(tick(true,f.pouch).event==Event::None);
    f.knifeNear=false;tick(false,f.pouch);assert(tick(true,f.pouch).event==Event::GrabFresh);
    assert(tick(false,{9,9,9}).event==Event::Discarded); // no insertion from far away
    // Idle position alone, held grip on entry, or reserve pickup cannot commit.
    s={};assert(tick(true,f.socket).event==Event::None);
    for(int i=0;i<100;++i)assert(!tick(true,f.socket).requestNative);
    s={};f.capacity=-1;tick(false,f.socket);assert(tick(true,f.socket).event==Event::None);
    f.capacity=6;
    // Capacity changes are not an acknowledged reload, even if counts balance.
    s={};s.phase=Phase::WaitingNative;s.startClip=2;s.startReserve=25;s.startCapacity=6;
    s.since=f.nowMs;f.clip=7;f.reserve=20;f.capacity=8;
    assert(tick(false,f.socket).event==Event::Cancelled);
    // A valid-looking but stale HUD sample never acknowledges or starts work.
    s={};s.phase=Phase::WaitingNative;s.startClip=2;s.startReserve=25;s.startCapacity=6;
    f.capacity=6;f.clip=6;f.reserve=21;f.ammoFresh=false;
    assert(tick(false,f.socket).event==Event::Cancelled);
    f.ammoFresh=true;
    AmmoObservation sample;const int counts[3]{2,25,6};
    assert(!sample.Fresh(20,10,30,100));sample.Capture(20,10,30,counts,100);
    assert(sample.Fresh(20,10,30,100)&&sample.Fresh(20,10,30,350));
    assert(!sample.Fresh(20,10,30,351)&&!sample.Fresh(20,10,30,99));
    assert(!sample.Fresh(21,10,30,100)&&!sample.Fresh(20,11,30,100));
    assert(!sample.Fresh(20,10,31,100)&&!sample.Fresh(0,10,30,100));
    sample.Clear();assert(!sample.Fresh(20,10,30,100)&&sample.counts[0]==-1);
    // Trigger pressed while carrying must be released before TMD can fire.
    hipknife::TriggerGate trigger;
    assert(trigger.Update(true,.8f)==0);
    assert(trigger.Update(false,.8f)==0);
    assert(trigger.Update(false,0)==0);
    assert(trigger.Update(false,.8f)==.8f);
    const auto p=Pouch({0,1.1f,0},0);assert(p.p.x<0&&p.p.y<1.1f&&p.p.z>0);
    const auto turned=Pouch({0,1.1f,0},3.14159265f/2);
    assert(std::fabs(turned.p.x-.18f)<1e-5f);
    assert(std::fabs(turned.p.z-.22f)<1e-5f);
    std::puts("centurion reload L1 PASS: sequence/ownership/cancel/ammo/tap/shoulder");
}
