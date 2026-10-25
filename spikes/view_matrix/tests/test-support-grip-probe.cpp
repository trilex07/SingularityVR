#include "../support_grip_probe.h"
#include "../grip_latch.h"
#include <cassert>
#include <cstring>
#include <limits>
#include <initializer_list>
#include <cstdio>

using supportprobe::Sample;
using supportprobe::Event;
static Sample Valid() {
    Sample s;s.weapon=123;s.pawn=456;s.vertices=11387;s.hands=2;
    s.raw=s.effective=s.tracked=s.weaponContext=s.enabled=s.gameplay=s.presented=true;
    s.delta[2]=-.3f;s.forward[2]=-1.f;
    s.acquire=TwoHandGripAllowed(s.delta,s.forward,s.effective,s.tracked,false);
    return s;
}
static void Gate(const Sample& s,const char* expected){assert(!std::strcmp(supportprobe::Gate(s),expected));}
int main(){
    Sample s=Valid();Gate(s,"eligible-unlatched");
    s.active=true;Gate(s,"latched");s=Valid();
    s.raw=false;Gate(s,"released");s=Valid();s.effective=false;
    s.reload=true;Gate(s,"reload-owned");s.reload=false;s.cylinder=true;Gate(s,"cylinder-active");
    s.knife=true;Gate(s,"knife-owned");s=Valid();s.tracked=false;Gate(s,"tracking");
    s=Valid();s.weaponContext=false;s.presented=false;Gate(s,"weapon-not-presented");
    s=Valid();s.weaponContext=false;s.gameplay=false;Gate(s,"not-gameplay");
    s=Valid();s.weaponContext=false;s.holstered=true;Gate(s,"holstered");
    s=Valid();s.weaponContext=false;s.paused=true;Gate(s,"paused");
    s=Valid();s.weaponContext=false;s.menu=true;Gate(s,"vr-menu");
    s=Valid();s.weaponContext=false;s.enabled=false;Gate(s,"disabled");

    // Diagnostic labels agree with the REAL production geometric predicate
    // for pistol, ordinary long guns and Seeker, including the opt-in near zone.
    for(unsigned gun:{5799u,8297u,11387u,11389u,12444u})
    for(bool nearZone:{false,true})
    for(int x=-65;x<=65;x+=5)for(int y=-65;y<=65;y+=5)for(int z=-65;z<=65;z+=5){
        s=Valid();s.vertices=gun;s.pistol=gun==5799;
        s.closeLongGun=nearZone&&(gun==8297u||gun==11387u||gun==11389u);
        s.delta[0]=x*.01f;s.delta[1]=y*.01f;s.delta[2]=z*.01f;
        s.acquire=TwoHandGripAllowed(s.delta,s.forward,true,true,false,s.pistol,s.closeLongGun);
        const bool diagnosticAccept=!std::strcmp(supportprobe::Gate(s),"eligible-unlatched");
        assert(s.acquire==diagnosticAccept);
        if(!s.acquire)assert(std::strcmp(supportprobe::Gate(s),"geometry-unclassified"));
    }
    // Physical Volk probe: a tracked, pressed grip 16.8 cm from the primary
    // controller failed only the raw forward-cone gate (alignment -0.2038).
    // The ordinary route stays unchanged; the scoped near zone accepts it.
    s=Valid();s.vertices=8297;s.delta[0]=.1645f;s.delta[1]=0;
    s.delta[2]=.0341f;s.acquire=TwoHandGripAllowed(s.delta,s.forward,true,true,false);
    assert(!s.acquire);
    s.closeLongGun=true;
    s.acquire=TwoHandGripAllowed(s.delta,s.forward,true,true,false,false,true);
    assert(s.acquire);
    assert(!TwoHandGripAllowed(s.delta,s.forward,false,true,false,false,true));
    assert(!TwoHandGripAllowed(s.delta,s.forward,true,false,false,false,true));
    // The same direction well away from the hand stays outside the near zone.
    const float distant[3]={.40f,0,.08f};
    assert(!TwoHandGripAllowed(distant,s.forward,true,true,false,false,true));
    // Seeker keeps its separate back/support routing.
    assert(!TwoHandGripAllowed(s.delta,s.forward,true,true,false,false,false));
    // Two valid Volk Grip presses in the archived PSVR2 log missed ONLY the
    // old 12 cm floor: 10.15 cm and 11.94 cm, both with tracked controllers.
    s=Valid();s.vertices=8297;s.closeLongGun=true;
    s.delta[0]=-.0128f;s.delta[1]=.0984f;s.delta[2]=.0216f;
    s.forward[0]=-.6506f;s.forward[1]=-.2291f;s.forward[2]=-.7241f;
    s.acquire=TwoHandGripAllowed(s.delta,s.forward,true,true,false,false,true);
    assert(s.acquire);Gate(s,"eligible-unlatched");
    assert(!TwoHandGripAllowed(s.delta,s.forward,true,true,false,false,false));
    s=Valid();s.delta[0]=std::numeric_limits<float>::quiet_NaN();s.acquire=false;Gate(s,"invalid-distance");
    // Observe exactly the production latch order: identity reset, acquire,
    // distance-independent hold, release. Observation cannot change the latch.
    supportprobe::Budget budget;griplatch::GripLatch latch,control;
    unsigned counts[4]{};
    for(unsigned gesture=0;gesture<30;++gesture){
        for(unsigned frame=0;frame<100;++frame){
            s=Valid();s.frame=gesture*100+frame;s.raw=s.effective=frame!=99;
            if(frame>=50)s.delta[2]=-2.f;
            s.acquire=TwoHandGripAllowed(s.delta,s.forward,s.effective,s.tracked,false);
            latch.Update(s.effective,s.acquire?0.f:1e9f,s.tracked,s.weapon,s.frame*11);
            control.Update(s.effective,s.acquire?0.f:1e9f,s.tracked,s.weapon,s.frame*11);
            s.active=s.effective&&latch.Latched();
            const auto r=budget.Observe(true,s);++counts[(unsigned)r.event];
            assert(latch.Latched()==control.Latched());
            if(r.event==Event::Release){assert(r.sample.raw);assert(r.sample.active);assert(r.sample.Distance()>1.f);}
        }
    }
    assert(counts[(unsigned)Event::Press]==24);
    assert(counts[(unsigned)Event::Acquired]==1); // Subsequent presses immediately latch.
    assert(counts[(unsigned)Event::Release]==24);
    supportprobe::Budget disabled;
    s=Valid();assert(disabled.Observe(false,s).event==Event::None);
    assert(disabled.Observe(true,s).event==Event::None); // No invented rising edge on opt-in.
    s.raw=false;assert(disabled.Observe(true,s).event==Event::None);
    puts("support grip probe: PASS (production gate matrix, measured Volk attempt, bounded receipts)");
}
