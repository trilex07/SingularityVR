#define NOMINMAX
#include "../centurion_reload.h"
#include <cassert>
#include <cstdio>
using namespace centurionreload;
int main(){
    unsigned cases=0;
    // AmmoCountsValid boundaries (session evidence: -1/-2/-1 must refuse).
    assert(AmmoCountsValid(5,16,6)&&++cases);
    assert(AmmoCountsValid(0,0,1)&&++cases);
    assert(AmmoCountsValid(6,16,6)&&++cases);
    assert(!AmmoCountsValid(-1,16,6)&&++cases);
    assert(!AmmoCountsValid(5,-2,6)&&++cases);
    assert(!AmmoCountsValid(5,16,-1)&&++cases);
    assert(!AmmoCountsValid(5,16,0)&&++cases);
    assert(!AmmoCountsValid(7,16,6)&&++cases);
    assert(!AmmoCountsValid(5,16,1001)&&++cases);
    assert(!AmmoCountsValid(5,1000001,6)&&++cases);
    {Frame f;f.clip=5;f.reserve=16;f.capacity=6;assert(AmmoValid(f)&&++cases);}
    {Frame f;f.clip=-1;assert(!AmmoValid(f)&&++cases);}
    // Policy: 49 invalid silent, 50th triggers once.
    HudReseatPolicy p;std::uint64_t t=10000;
    for(unsigned i=0;i<49;++i){assert(!p.NoteSample(false,t+=100)&&++cases);}
    assert(p.NoteSample(false,t+=100)&&++cases);
    assert(p.reseats==1);
    // Cooldown blocks the immediate next epoch.
    for(unsigned i=0;i<49;++i){assert(!p.NoteSample(false,t+=100)&&++cases);}
    assert(!p.NoteSample(false,t+=50)&&++cases); // 50th but inside cooldown
    t+=6000;assert(p.NoteSample(false,t)&&++cases); // cooldown passed
    assert(p.reseats==2);
    // A valid read opens a new epoch (reseat budget restored).
    assert(!p.NoteSample(true,t+=100)&&++cases);
    assert(p.reseats==0&&p.bad==0);
    // Ineligible gaps never accumulate.
    HudReseatPolicy q;t=50000;
    for(unsigned i=0;i<30;++i)q.NoteSample(false,t+=100);
    q.NoteIneligible();++cases;
    for(unsigned i=0;i<20;++i){assert(!q.NoteSample(false,t+=100)&&++cases);}
    // Cap: 6 reseats per epoch, then quiet until a new epoch.
    HudReseatPolicy r;t=100000;
    for(unsigned epoch=0;epoch<6;++epoch){
        for(unsigned i=0;i<49;++i)r.NoteSample(false,t+=100);
        t+=6000;assert(r.NoteSample(false,t)&&++cases);
    }
    assert(r.reseats==6);
    for(unsigned i=0;i<60;++i){t+=100;assert(!r.NoteSample(false,t)&&++cases);}
    r.ResetEpoch();++cases;
    for(unsigned i=0;i<49;++i)r.NoteSample(false,t+=100);
    t+=6000;assert(r.NoteSample(false,t)&&++cases);
    printf("PASS hud reseat: %u cases; trigger/cooldown/cap/epoch/ineligible\n",cases);
    return 0;
}
