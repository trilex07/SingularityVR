#include "support/gpu_fence_wait.h"
#include <cassert>
#include <cstdio>
int main(){
    unsigned poll=0,relax=0,yield=0;
    auto a=gpufence::Wait(false,[&](bool flush){assert(flush);return ++poll<5?1:0;},[]{return 2.;},[&]{++relax;},[&]{++yield;return true;});
    assert(a.status==0&&!a.timeout&&a.polls==5&&relax==4&&!yield);
    poll=relax=yield=0;
    auto b=gpufence::Wait(true,[&](bool){return ++poll<5?1:0;},[&]{return poll*.05;},[&]{++relax;},[&]{++yield;return true;});
    assert(b.status==0&&!b.timeout&&b.polls==5&&relax==1&&yield==3&&b.switched==3);
    poll=relax=yield=0;
    auto c=gpufence::Wait(true,[&](bool){return ++poll<5?1:0;},[]{return 1.;},[&]{++relax;},[&]{++yield;return false;});
    assert(c.status==0&&!c.timeout&&c.polls==5&&relax==4&&yield==4&&!c.switched);
    poll=0;
    auto d=gpufence::Wait(true,[&](bool){++poll;return 1;},[]{return 101.;},[]{assert(false);},[]{assert(false);return true;});
    assert(d.timeout&&d.status==1&&poll==1);
    auto e=gpufence::Wait(true,[](bool){return -2005530520;},[]{assert(false);return 0.;},[]{assert(false);},[]{assert(false);return false;});
    assert(e.status<0&&!e.timeout&&e.polls==1&&!e.yields);
    poll=0;
    auto f=gpufence::Wait(false,[&](bool flush){assert(flush==(poll%64==0));return ++poll<130?1:0;},
        []{return 2.;},[]{},[]{assert(false);return false;},true);
    assert(f.status==0&&f.polls==130&&f.flushes==3&&!f.yields);
    puts("GPU fence wait: PASS (completion,100us spin,no-peer fallback,100ms deadline,error,periodic flush)");
}
