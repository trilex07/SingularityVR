#include "../hand_anchor_probe.h"
#include <cassert>
#include <limits>
#include <cstdio>
int main(){
    handanchor::Probe p;handanchor::Record r,out;
    r.tick=0xfffffff0u;
    r.local[0]=std::numeric_limits<float>::quiet_NaN();
    assert(!p.Capture(r)&&p.Count()==0);
    r.local[0]=1;assert(p.Capture(r));assert(!p.Capture(r));
    assert(p.Take(out)&&out.tick==r.tick&&out.local[0]==1);assert(!p.Take(out));
    r.tick+=1999;assert(!p.Capture(r));++r.tick;
    for(unsigned n=1;n<handanchor::Probe::limit;++n){assert(p.Capture(r));assert(p.Take(out));r.tick+=2000;}
    assert(p.Count()==8&&!p.Capture(r));
    handanchor::WeaponProbe wp;
    r.tick=100;
    assert(wp.Due(r.tick,0)&&wp.Capture(r));
    assert(!wp.Due(r.tick,0x1234)); // don't lose a pending free-hand record
    assert(wp.Take(out)&&out.weapon==0&&out.epoch==1);
    for(unsigned n=1;n<8;++n){r.tick+=2000;assert(wp.Due(r.tick,0)&&wp.Capture(r)&&wp.Take(out));}
    assert(!wp.Due(r.tick,0));
    assert(wp.Due(r.tick,0x1234)&&wp.Capture(r)&&wp.Take(out));
    assert(out.weapon==0x1234&&out.epoch==2);
    for(unsigned n=1;n<8;++n){r.tick+=2000;assert(wp.Due(r.tick,0x1234)&&wp.Capture(r)&&wp.Take(out));}
    assert(wp.Count()==16&&!wp.Due(r.tick+4000,0x1234));
    assert(!wp.Due(r.tick+4000,0x5678)); // fixed budget, not eight records per every swap
    handanchor::Probe tmd[2];r.tmd=true;r.tick=100;r.eye=0;
    assert(!tmd[0].Capture(r)); // no bound IB/face count/units
    r.indexBuffer=0x1234;r.triangles=4402;r.uuPerMetre=65.625f;
    r.wristWorld[0][0]=std::numeric_limits<float>::quiet_NaN();
    assert(!tmd[0].Capture(r));r.wristWorld[0][0]=12;
    r.verts=7444;r.hands=2;r.support=0;
    for(unsigned i=0;i<8;++i){
        for(unsigned eye=0;eye<2;++eye){r.eye=eye;assert(tmd[eye].Capture(r));
            assert(tmd[eye].Take(out));assert(out.eye==eye&&out.tmd&&out.indexBuffer==0x1234);
            assert(out.wristWorld[0][0]==12&&out.uuPerMetre==65.625f);}
        r.tick+=2000;
    }
    for(auto& p:tmd)assert(!p.Capture(r)); // independent bounded stereo budgets
    std::puts("PASS: fixed eight records, two-second cadence/wrap, pending isolation, finite inputs, final-bound TMD stereo metadata");
}
