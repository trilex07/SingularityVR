#include "../wrist_equipment_source_policy.h"
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdint>
int main(){
    using wristhud::SourceCurrent;
    constexpr std::uintptr_t ctl=0x1000,pawn=0x2000,hud=0x3000,klass=0x4000;
    assert(wristhud::SourceOwner(ctl,pawn,ctl,pawn));
    assert(!wristhud::SourceOwner(ctl,0,ctl,pawn));
    assert(!wristhud::SourceOwner(ctl,pawn,ctl+4,pawn));
    assert(!wristhud::SourceOwner(ctl,pawn,ctl,pawn+4));
    assert(SourceCurrent(ctl,pawn,hud,klass,1000,ctl,pawn,klass,1100));
    assert(!SourceCurrent(ctl,pawn,hud,klass,1000,ctl,pawn,klass,1251));
    assert(!SourceCurrent(ctl,pawn,hud,klass,1000,ctl,0x2004,klass,1100));
    assert(!SourceCurrent(ctl,pawn,hud,klass,1000,0x1004,pawn,klass,1100));
    assert(!SourceCurrent(ctl,pawn,hud,klass,1000,ctl,pawn,0x4004,1100));
    assert(!SourceCurrent(ctl,pawn,0,klass,1000,ctl,pawn,klass,1100));
    // Before: four property-chain lookups on a render-thread HUD class change.
    // After: zero; only a fixed-size snapshot identity test is done there.
    // This bounded synthetic benchmark is a cost shape comparison, not a
    // measured game frame time or a substitute for MAIN's profiler.
    volatile unsigned sink=0;
    volatile std::uintptr_t observedController=ctl,observedPawn=pawn,observedClass=klass;
    constexpr int rounds=100000;
    const auto a=std::chrono::steady_clock::now();
    for(int i=0;i<rounds;++i){
        for(int field=0;field<4;++field)for(int node=0;node<64;++node)
            sink+=((unsigned)field+(unsigned)node)&1u;
    }
    const auto b=std::chrono::steady_clock::now();
    for(int i=0;i<rounds;++i)
        sink+=SourceCurrent(ctl,pawn,hud,klass,1000,observedController,
                            observedPawn,observedClass,1100)?1u:0u;
    const auto c=std::chrono::steady_clock::now();
    const double oldNs=std::chrono::duration<double,std::nano>(b-a).count()/rounds;
    const double newNs=std::chrono::duration<double,std::nano>(c-b).count()/rounds;
    std::printf("PASS source lifecycle; synthetic class-change cost %.1f -> %.1f ns (sink=%u)\n",
                oldNs,newNs,sink);
}
