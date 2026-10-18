#include "../texture_bind_decision.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
struct Result {bool stage=false,protectedSource=false;unsigned candidates=0,calls=0,misses=0,requests=0;};
static Result Run(bool optimize,bool vr,bool vrShadows,bool bridge,unsigned stage,bool shadow,bool menu){
    Result r;texturebind::Decision decision(optimize);bool known=false;
    auto projected=[&]{return decision.Get([&]{++r.calls;if(!known){known=true;++r.misses;if(shadow)++r.candidates;}return shadow;});};
    if(vr&&!vrShadows&&projected())++r.candidates;
    if(vr&&stage<16)r.stage=projected();
    if(bridge)r.protectedSource=projected()||menu;
    r.requests=decision.requests;return r;
}
int main(){
    unsigned avoided=0;
    for(unsigned flags=0;flags<32;++flags)for(unsigned stage=0;stage<21;++stage){
        const bool vr=flags&1,vrShadows=flags&2,bridge=flags&4,shadow=flags&8,menu=flags&16;
        auto old=Run(false,vr,vrShadows,bridge,stage,shadow,menu);
        auto now=Run(true,vr,vrShadows,bridge,stage,shadow,menu);
        assert(old.stage==now.stage&&old.protectedSource==now.protectedSource&&old.candidates==now.candidates);
        assert(now.calls<=1&&old.requests==now.requests&&old.misses==now.misses);
        avoided+=old.calls-now.calls;
    }
    assert(avoided>0);
    // Different/reused resources in subsequent invocations must reclassify.
    unsigned calls=0;
    for(bool value:{true,false,true}){texturebind::Decision d(true);
        assert(d.Get([&]{++calls;return value;})==value);assert(d.Get([&]{assert(false);return false;})==value);}
    assert(calls==3);
    auto a=Run(false,true,false,true,0,true,false),b=Run(true,true,false,true,0,true,false);
    assert(a.calls==3&&b.calls==1&&a.candidates==b.candidates);
    std::printf("texture bind: PASS, 672 combinations, %u repeated classifications removed; 3->1 on triple path\n",avoided);
}
