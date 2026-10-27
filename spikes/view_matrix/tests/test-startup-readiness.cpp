#include "../startup_readiness.h"
#include <cassert>
#include <cstdio>
int main(){
    startup::Retry retry;
    assert(!retry.Due(1000,false,true)&&!retry.Due(1000,true,false)&&!retry.attempts);
    assert(retry.Due(1000,true,true)&&!retry.Due(1249,true,true));
    assert(retry.Due(1250,true,true)&&retry.Due(1500,true,true)&&retry.Due(1750,true,true));
    assert(!retry.Due(2749,true,true)&&retry.Due(2750,true,true));
    for(unsigned i=0;i<20;++i)retry.Due(5000+i*1000,true,true);
    assert(retry.attempts==12&&!retry.Due(999999,true,true));
    for(unsigned missing=0;missing<6;++missing){
        startup::Curtain c;const auto bits=startup::All&~(1u<<missing);
        assert(c.Update(true,1,bits,1000));assert(c.Update(true,1,bits,5999));
        assert(!c.Update(true,1,bits,6000)&&c.timedOut&&c.done);
        assert(!c.Update(true,1,bits,9000)); // no indefinite curtain
    }
    startup::Curtain c;
    assert(c.Update(true,0,0,1000));assert(c.Update(true,1,startup::All,1200));
    assert(c.Update(true,1,startup::All,1299));assert(!c.Update(true,1,startup::All,1300)&&!c.timedOut);
    assert(!c.Update(false,1,0,1400));assert(!c.Update(true,1,0,1500)); // pause/tracking loss cannot re-cover live play
    assert(c.Update(true,2,0,1600));assert(c.Update(true,2,startup::All,1700));
    assert(c.Update(true,2,0,1750));assert(c.Update(true,2,startup::All,1800));
    assert(c.Update(true,2,startup::All,1899));assert(!c.Update(true,2,startup::All,1900));
    startup::Curtain menu;
    for(unsigned t=0;t<20000;t+=50)assert(!menu.Update(false,0,0,t));
    assert(menu.Update(true,1,0,20000));assert(!menu.Update(false,1,0,20001)); // immediate menu/cine escape
    assert(menu.Update(true,1,0,30000));assert(!menu.Update(true,1,0,29999)&&menu.timedOut); // clock fault fails open
    startup::Curtain clamp;assert(clamp.Update(true,1,0,1000,0));assert(!clamp.Update(true,1,0,1500,0));
    startup::Curtain minHold;
    assert(minHold.Update(true,1,startup::All,1000,5000,2000));
    assert(minHold.Update(true,1,startup::All,1099,5000,2000));
    assert(minHold.Update(true,1,startup::All,2999,5000,2000));
    assert(!minHold.Update(true,1,startup::All,3000,5000,2000)&&!minHold.timedOut);
    startup::Curtain lateReady;
    assert(lateReady.Update(true,1,0,1000,5000,2000));
    assert(lateReady.Update(true,1,startup::All,2999,5000,2000));
    assert(!lateReady.Update(true,1,startup::All,3099,5000,2000));
    startup::Curtain bounded;
    assert(bounded.Update(true,1,0,1000,3000,5000));
    assert(!bounded.Update(true,1,0,4000,3000,5000)&&bounded.timedOut);
    puts("PASS startup policy: wall-clock retry cap,100ms stable, each missing gate, timeout/fail-open, pause/cine escape, identity rearm");
}
