#include "../seeker_ammo_observer.h"
#include <cassert>
#include <cstdio>
int main(){using namespace seekerammo;using C=nativerefill::Counts;
    Transaction t;assert(!t.Begin(0,2,{0,5,1},100));assert(!t.Begin(1,2,{1,5,1},100));
    assert(t.Begin(1,2,{0,5,1},100));assert(!t.Due(199));assert(t.Due(200));
    assert(t.Sample(1,2,{0,5,1},200)==Observation::Waiting);
    t.accepted=1;assert(t.Sample(1,2,{1,4,1},300)==Observation::Conserved);assert(!t.active);
    assert(t.Begin(1,2,{0,5,1},1000));t.accepted=1;
    assert(t.Sample(1,3,{1,4,1},1100)==Observation::Inconclusive);
    assert(t.Begin(1,2,{0,5,1},2000));t.accepted=1;
    assert(t.Sample(1,2,{1,5,1},2100)==Observation::Inconclusive); // duplication rejected
    assert(t.Begin(1,2,{0,5,1},3000));t.accepted=0;
    assert(t.Sample(1,2,{1,4,1},3100)==Observation::Inconclusive); // no successful native request
    assert(t.Begin(1,2,{0,5,1},4000));t.accepted=1;
    assert(t.Sample(1,2,{0,5,1},12000)==Observation::Expired);
    assert(t.Begin(1,2,C{0,5,1},13000));assert(t.Sample(1,2,C{},13100)==Observation::Inconclusive);
    puts("PASS Seeker read-only transaction: identity, request/ack, exact conservation, duplicate, invalid, timeout gates");
}
