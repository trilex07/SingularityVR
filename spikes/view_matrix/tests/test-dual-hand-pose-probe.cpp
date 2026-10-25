#include "../dual_hand_pose_probe.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
int main(){
    dualpose::Budget b;dualpose::State s{2,0,0,5799,123};
    assert(!b.Observe(false,1,0,s));assert(b.Transition()==0);
    assert(b.Observe(true,1,0,s));assert(b.Phase()==0);
    for(unsigned p=0;p<3;++p)for(unsigned e=0;e<2;++e){
        assert(b.Claim(1,p,e));assert(!b.Claim(1,p,e));}
    assert(!b.Claim(0,0,0)&&!b.Claim(1,3,0)&&!b.Claim(1,0,2));
    b.EndFrame();assert(!b.Observe(true,1,1,s));
    assert(!b.Observe(true,2,249,s));assert(b.Observe(true,3,250,s)&&b.Phase()==1);
    b.EndFrame();assert(!b.Observe(true,4,900000,s)); // unlimited idle cannot spend budget
    s.actions=1;assert(b.Observe(true,5,900001,s)&&b.Transition()==2);
    // A changed event inside a frame cannot mix states between the two eyes.
    auto other=s;other.actions=4;
    assert(b.Observe(true,5,900002,other)&&b.Current().actions==1);
    assert(b.Claim(5,0,0)&&b.Claim(5,0,1));
    unsigned frame=6,tick=900005;
    for(unsigned i=2;i<dualpose::Budget::maxTransitions;++i){
        s.actions=i;assert(b.Observe(true,frame++,tick++,s));b.EndFrame();
    }
    assert(b.Observe(true,frame++,tick+300,s)&&b.Phase()==1);b.EndFrame();
    s.support=1;assert(!b.Observe(true,frame++,tick+400,s));
    dualpose::Budget wrap;
    assert(wrap.Observe(true,0,0xffffff80u,s));wrap.EndFrame();
    assert(!wrap.Observe(true,1,0x00000079u,s));
    assert(wrap.Observe(true,2,0x0000007au,s)); // uint32 time wrap
    dualpose::Budget states;
    frame=0;
    for(unsigned action: {0u,1u,0u,2u,0u,4u,0u}){
        s.actions=action;assert(states.Observe(true,++frame,frame*100,s));states.EndFrame();}
    s.support=1;assert(states.Observe(true,++frame,1000,s));states.EndFrame();
    s.hands=1;assert(states.Observe(true,++frame,1001,s));states.EndFrame();
    s.weapon=456;assert(states.Observe(true,++frame,1002,s));
    std::puts("PASS: pose budget OFF, event/settled, paired eyes, stale/duplicate rejection, idle, bounded exhaustion, wrap and action/support/ownership changes");
}
