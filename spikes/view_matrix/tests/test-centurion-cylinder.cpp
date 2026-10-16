#include "../centurion_clip_preview.h"
#include <cassert>
#include <cstdio>
using namespace centurionreload;
int main(){
    State s;Config c;c.cylinderGestures=true;
    Frame f;f.enabled=f.context=f.tracked=f.supported=f.ammoFresh=true;
    f.pawn=20;f.weapon=10;f.clip=2;f.reserve=25;f.capacity=6;
    f.socket={.2f,1,-.5f};f.gun={f.socket,{}};
    f.pouch=Pouch({0,1.1f,0},0).p;f.hand=f.socket;f.nowMs=1000;
    auto tick=[&](unsigned dt=20){f.nowMs+=dt;return Update(s,f,c);};
    tick();f.grip=true;assert(tick().event==Event::None); // no old-grip acquisition
    f.grip=false;tick();f.releaseCylinder=true;
    assert(tick().event==Event::Unlocked);f.releaseCylinder=false;
    for(unsigned i=0;i<100;++i)assert(!tick().requestNative); // no free reload
    f.gun.p.z-=.3f;assert(tick().event==Event::None); // forward motion != opening
    f.gun.p.x+=.08f;assert(tick().event==Event::Opened);
    for(unsigned i=0;i<10;++i)assert(!tick().requestNative);
    f.gun.q=centurionclip::Degrees(-45,0,0);
    assert(tick().event==Event::None); // sustained down, not one pose
    assert(tick(100).event==Event::Dumped&&s.phase==Phase::NeedAmmo);
    // Only the physical left shoulder supplies the fresh cylinder.
    f.grip=true;f.hand=hipknife::Hip({0,1.1f,0},0).p;f.knifeNear=true;
    assert(tick().event==Event::None);
    f.grip=false;f.knifeNear=false;tick();f.grip=true;f.hand=f.pouch;
    assert(tick().event==Event::GrabFresh);
    f.hand=f.socket;tick();f.grip=false;
    assert(tick().event==Event::Loaded&&!tick().requestNative);
    f.gun.q={};assert(!tick(100).requestNative); // pitch back alone != close
    f.gun.p.x-=.08f;
    assert(tick().requestNative&&s.phase==Phase::WaitingNative);
    for(unsigned i=0;i<100;++i)assert(!tick().requestNative);
    f.clip=6;f.reserve=21;assert(tick().event==Event::Acknowledged);
    // Every added state cancels on focus/tracking/swap/ammo loss, never commits.
    for(auto phase:{Phase::Unlocked,Phase::Open,Phase::NeedAmmo,Phase::Loaded}){
        for(unsigned bad=0;bad<4;++bad){
            s={};s.phase=phase;s.pawn=20;s.weapon=10;s.since=f.nowMs;
            s.startClip=2;s.startReserve=25;s.startCapacity=6;f.clip=2;f.reserve=25;
            f.context=f.tracked=f.ammoFresh=true;f.weapon=10;
            if(bad==0)f.context=false;if(bad==1)f.tracked=false;
            if(bad==2)f.ammoFresh=false;if(bad==3)f.weapon=11;
            const auto r=tick();assert(r.event==Event::Cancelled&&!r.requestNative);
        }
    }
    // A full cylinder does not begin, and a circle press alone never spends ammo.
    s={};f.context=f.tracked=f.ammoFresh=true;f.weapon=10;f.clip=6;
    f.releaseCylinder=true;assert(tick().event==Event::None&&f.clip==6&&f.reserve==25);
    std::puts("Centurion cylinder L1 PASS: circle/open/dump/shoulder/insert/flick/one native request/cancel");
}
