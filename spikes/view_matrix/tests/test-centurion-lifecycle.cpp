#include "../centurion_reload.h"
#include <cassert>
#include <cstdio>
#include <limits>
using namespace centurionreload;

int main(){
    // Deterministic lifecycle / action replay, not an engine or headset test.
    // Structured gestures exercise every transition while random interruptions
    // target tracking, identity, focus, clock, reserve and off-hand ownership.
    std::uint32_t random=0xcef179u;
    auto next=[&](){random=random*1664525u+1013904223u;return random;};
    unsigned inserted=0,acknowledged=0,cancelled=0;
    for(unsigned run=0;run<10000;++run){
        State s;Frame f;f.enabled=f.context=f.tracked=f.supported=f.ammoFresh=true;
        f.pawn=20;f.weapon=10;f.clip=2;f.reserve=25;f.capacity=6;
        f.socket={.2f,1,-.5f};f.pouch={0,.6f,-.34f};f.hand=f.socket;
        f.nowMs=1000;
        for(unsigned frame=0;frame<36;++frame){
            f.nowMs+=20;f.grip=false;f.hand=f.socket;
            if(frame==1){f.grip=true;}
            if(frame==2){f.grip=true;f.hand={.2f,1,-.7f};}
            if(frame==3){f.hand={.2f,1,-.7f};}
            if(frame==4){f.grip=true;f.hand=f.pouch;}
            if(frame==5){f.grip=true;}
            if(frame>=8){f.clip=6;f.reserve=21;}
            // 1/4 of runs remain a complete positive control; the others get
            // one interruption at a random boundary, including held squeezes.
            if(run%4&&frame==run%18){
                switch(next()%12){
                case 0:f.context=false;break;
                case 1:f.tracked=false;break;
                case 2:f.weapon++;break;
                case 3:f.pawn++;break;
                case 4:f.ammoFresh=false;break;
                case 5:f.handBusy=true;break;
                case 6:f.supportLatched=true;break;
                case 7:f.firing=true;break;
                case 8:f.hand.x=std::numeric_limits<float>::infinity();break;
                case 9:f.nowMs=0;break;
                case 10:f.capacity=8;break;
                case 11:f.reserve+=4;break;
                }
            }
            const State before=s;const int clip=f.clip,reserve=f.reserve;
            const auto result=Update(s,f);
            assert(f.clip==clip&&f.reserve==reserve); // observer, never inventory
            assert(!result.reserveHand||f.grip);
            if(result.requestNative){
                ++inserted;
                assert(result.event==Event::Inserted&&before.phase==Phase::Carrying);
                assert(before.priorGrip&&!f.grip&&f.context&&f.tracked&&f.ammoFresh);
                assert(f.weapon==before.weapon&&f.pawn==before.pawn);
                assert(Near(f.hand,f.socket,Config{}.socketRadius));
                assert(f.clip==before.startClip&&f.reserve==before.startReserve);
            }
            if(result.event==Event::Acknowledged){
                ++acknowledged;assert(before.phase==Phase::WaitingNative);
                const int gained=f.clip-before.startClip;
                assert(f.ammoFresh&&gained>0&&f.reserve==before.startReserve-gained);
                assert(f.capacity==before.startCapacity);
            }
            if(result.event==Event::Cancelled)++cancelled;
            f.context=f.tracked=f.ammoFresh=true;
            f.handBusy=f.supportLatched=f.firing=false;
        }
    }
    assert(inserted>=2500&&acknowledged>=2500&&cancelled>0);
    std::printf("centurion lifecycle L1 PASS: 360000 frames, inserts=%u acknowledgements=%u cancellations=%u\n",
        inserted,acknowledged,cancelled);
}
