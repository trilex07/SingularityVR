#include "../volk_skin_probe.h"
#include <cassert>
#include <cstdio>
#include <limits>
int main(){
    using namespace volkskin;
    static_assert(std::atomic<unsigned>::is_always_lock_free,"probe must not lock");
    assert(bones[0]==55&&bones[2]==53);
    assert(Signature(8297,0,10835,0,0));
    assert(!Signature(8297,1,10835,0,0)&&!Signature(8297,0,10835,0,1));
    Mailbox m;assert(m.Publish(ImpulseDown));assert(m.Publish(ImpulseUp));
    assert(m.Take()==(ImpulseDown|ImpulseUp));assert(m.Take()==0);
    Window w;assert(w.Draw(100,2,0));assert(!w.Draw(100,2,0));
    assert(w.Draw(101,2,0));assert(!w.Draw(102,2,0));assert(w.Draw(104,2,0));
    assert(w.Draw(124,2,0));assert(!w.Draw(125,2,0));
    // L2 can fire without hands changing. This was missed by the old probe.
    assert(w.Draw(130,2,ImpulseDown));assert(w.reason==ImpulseDown);
    assert(w.Draw(131,2,ImpulseUp|FireEnter|FireExit));
    for(unsigned i=140;i<400;++i)w.Draw(i,2,ImpulseDown);
    assert(w.captures==64&&!w.Draw(401,1,Hands));
    for(unsigned i=0;i<100;++i)m.Publish(FireEnter);
    assert(!m.Publish(FireExit));
    float p[12]={1,0,0,7,0,1,0,8,0,0,1,9};
    assert(Determinant(p)==1&&Finite(p,12));
    p[0]=p[5]=p[10]=0;assert(Determinant(p)==0&&Finite(p,12));
    p[0]=std::numeric_limits<float>::quiet_NaN();assert(!Finite(p,12));
    puts("PASS Volk five-bone probe: exact signature, event coalescing, unchanged hands L2, bounded windows, degeneracy");
}
