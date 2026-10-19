#include "../weapon_wheel_seek.h"
#include "../hand_anchor_math.h"
#include "../weapon_wheel_layout.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
#include <algorithm>
int main(){
    wheelseek::State s;
    assert(wheelseek::Begin(s,5799,8297,100));
    assert(wheelseek::Observe(s,5799,110,true)==wheelseek::Result::Wait);
    assert(wheelseek::Observe(s,11387,120,true)==wheelseek::Result::Pulse);
    assert(wheelseek::Observe(s,8297,130,true)==wheelseek::Result::Landed);
    // Same displayed target works when native inventory skips an unowned gun.
    assert(wheelseek::Begin(s,5799,8297,100));
    assert(wheelseek::Observe(s,8297,120,true)==wheelseek::Result::Landed);
    assert(wheelseek::Begin(s,5799,8297,100));
    assert(wheelseek::Observe(s,11387,110,true)==wheelseek::Result::Pulse);
    assert(wheelseek::Observe(s,5799,120,true)==wheelseek::Result::Failed); // not owned, full cycle
    assert(wheelseek::Begin(s,5799,8297,100));
    assert(wheelseek::Observe(s,0,5100,true)==wheelseek::Result::Failed);
    assert(wheelseek::Begin(s,5799,8297,100));
    assert(wheelseek::Observe(s,8297,101,false)==wheelseek::Result::Failed); // menu invalidates
    assert(!wheelseek::Begin(s,5799,5799,100));
    assert(!wheelseek::Begin(s,0,8297,100));
    const long all[4]={5799,11387,8297,11389};
    // Every carried subset, every native cycle permutation, every start and
    // target. The algorithm must not assume catalog cycle order or ownership.
    int permutation[4]={0,1,2,3};
    do {
        for(unsigned mask=1;mask<16;++mask){
            long cycle[4]{};unsigned count=0;
            for(auto index:permutation)if(mask&(1u<<index))cycle[count++]=all[index];
            for(unsigned start=0;start<count;++start)for(unsigned target=0;target<count;++target){
                if(start==target){assert(!wheelseek::Begin(s,cycle[start],cycle[target],100));continue;}
                assert(wheelseek::Begin(s,cycle[start],cycle[target],100));
                unsigned pos=start;
                for(unsigned steps=1;steps<count;++steps){
                    pos=(pos+1)%count;
                    auto result=wheelseek::Observe(s,cycle[pos],100+steps*100,true);
                    if(pos==target){assert(result==wheelseek::Result::Landed);break;}
                    assert(result==wheelseek::Result::Pulse);
                }
                assert(!s.target);
            }
        }
    } while(std::next_permutation(permutation,permutation+4));
    assert(handanchor::VolkSingleGrip(true,true,true,false,8297,false));
    for(int flag=0;flag<2;++flag)for(int gloves=0;gloves<2;++gloves)
    for(int live=0;live<2;++live)for(int holstered=0;holstered<2;++holstered)
    for(int support=0;support<2;++support)for(long verts:{5799L,8297L,11387L,11389L})
        assert(handanchor::VolkSingleGrip(flag!=0,gloves!=0,live!=0,holstered!=0,verts,support!=0)==
            bool(flag&&gloves&&live&&!holstered&&verts==8297&&!support));
    // XR positive Y is up; convert ONCE at the runtime boundary, not inside
    // screen-space helpers (HandSector already projects to Y-down).
    assert(weaponwheel::SectorForStick(0,-1,4)==0);
    assert(weaponwheel::SectorForStick(1,0,4)==1);
    assert(weaponwheel::SectorForStick(0,1,4)==2);
    assert(weaponwheel::SectorForStick(-1,0,4)==3);
    std::puts("PASS wheel observed transition seek, bounded failure, shotgun exact gates, screen directions");
}
