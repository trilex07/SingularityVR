#include "../test_weapon_grant_policy.h"
#include <cassert>
int main(){
    testweapongrant::Once g;
    assert(!g.Poll(false,true,6,100));assert(g.attempts==0);
    assert(!g.Poll(true,false,6,100));assert(!g.Poll(true,true,6,100));
    assert(!g.Poll(true,true,6,2599));assert(g.Poll(true,true,6,2600)==9);
    assert(!g.Poll(true,true,7,2601));assert(g.Poll(true,true,7,4600)==8);
    assert(!g.Poll(true,true,15,6600));assert(g.done);
    assert(!g.Poll(true,true,0,10000)); // no regrant after death/load in this session.
    testweapongrant::Once fail;
    for(unsigned i=1;i<40000;++i)fail.Poll(true,true,6,i);
    assert(fail.attempts==3); // failed native factory cannot flood or spawn endlessly.
    testweapongrant::Once changed;
    assert(!changed.Poll(true,true,6,100,42));
    assert(!changed.Poll(true,true,6,2700,43)); // new pawn needs its own stable interval.
    assert(changed.Poll(true,true,6,5200,43)==9);
}
