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
    std::puts("PASS: fixed eight records, two-second cadence/wrap, pending isolation, finite inputs");
}
