#include "../reload_controls.h"
#include <cassert>
#include <cstdio>
#include <limits>
#include <initializer_list>
using namespace reloadcontrols;
int main(){
    const Result mapped{true,true,true};
    auto route=RouteFace(true,true,Face::Cross,true,0x1000,false,mapped);
    assert(route.down&&route.health&&route.mask==0x0001);
    route=RouteFace(true,true,Face::Circle,true,0x0001,true,mapped);
    assert(!route.down&&!route.health&&route.mask==0);
    for(bool layout:{false,true})for(bool gameplay:{false,true})for(bool held:{false,true}){
        route=RouteFace(layout,gameplay,Face::Other,held,0x200,true,mapped);
        assert(route.down==held&&route.mask==0x200&&route.health);
        if(!layout||!gameplay){
            route=RouteFace(layout,gameplay,Face::Cross,held,0x1000,false,mapped);
            assert(route.down==held&&route.mask==0x1000&&!route.health);
            route=RouteFace(layout,gameplay,Face::Circle,held,0x0001,true,mapped);
            assert(route.down==held&&route.mask==0x0001&&route.health);
        }
    }
    State s;
    assert(!s.Update(true,true,true,0,1,1000).health); // held on entry
    assert(!s.Update(true,true,true,0,1,1020).releaseCylinder);
    s.Update(true,false,false,0,0,1040);
    auto r=s.Update(true,true,false,0,0,1060);
    assert(r.health&&!r.releaseCylinder&&!r.jump);
    r=s.Update(true,false,true,0,0,1080);
    assert(!r.health&&r.releaseCylinder&&!r.jump);
    assert(!s.Update(true,false,true,0,0,1100).releaseCylinder);
    r=s.Update(true,false,false,0,1,1120);
    assert(r.jump&&!r.health&&!r.releaseCylinder);
    assert(s.Update(true,false,false,0,1,1239).jump);
    for(unsigned i=1240;i<10000;i+=20)assert(!s.Update(true,false,false,0,1,i).jump);
    s.Update(true,false,false,0,0,10000);
    assert(s.Update(true,false,false,.2f,.9f,10020).jump);
    // No diagonal/sideways trigger and no jump on rotating a held snap input.
    s.Update(true,false,false,0,0,10200);
    assert(!s.Update(true,false,false,.9f,.9f,10220).jump);
    assert(!s.Update(true,false,false,0,.9f,10240).jump);
    s.Update(true,false,false,0,0,10260);
    assert(!s.Update(true,false,false,0,-1,10280).jump); // crouch is untouched
    // Invalid contexts cancel even a pending jump pulse and demand real neutral
    // input afterward, not a synthetic release from inactive XR bindings.
    s.Update(true,false,false,0,0,10300);
    assert(s.Update(true,false,false,0,1,10320).jump);
    assert(!s.Update(false,false,false,0,0,10340).jump);
    r=s.Update(true,true,true,0,1,10360);
    assert(!r.jump&&!r.health&&!r.releaseCylinder);
    s.Update(true,false,false,0,0,10380);
    r=s.Update(true,true,true,0,1,10400);
    assert(r.health&&r.releaseCylinder&&r.jump);
    r=s.Update(true,true,true,0,1,500); // clock regression
    assert(!r.health&&!r.releaseCylinder&&!r.jump);
    s.Update(true,false,false,0,0,520);
    r=s.Update(true,false,false,std::numeric_limits<float>::quiet_NaN(),1,540);
    assert(!r.jump);
    std::puts("reload controls L1 PASS: Cross health/Circle release/stick jump/pulse/neutral/context/NaN");
}
