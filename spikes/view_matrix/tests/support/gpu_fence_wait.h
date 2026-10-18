#pragma once
// OFFLINE EXPERIMENT ONLY. PID22160 follow-up benchmark found no latency/CPU
// benefit; neither policy is integrated into the game renderer.
#include <cstdint>
namespace gpufence {
struct Result {long status=1;bool timeout=false;std::uint64_t polls=0,flushes=0,yields=0,switched=0;};
// The event query is still the authority. No frame is deemed complete merely
// because a yield occurred. Default policy spins; cooperative mode yields only
// after100us. Uses the existing100ms deadline and never requests a timed sleep.
template<class Poll,class Clock,class Relax,class Yield>
Result Wait(bool cooperative,Poll poll,Clock elapsedMs,Relax relax,Yield yield,bool coalesceFlush=false){
    Result r;
    while(true){
        const bool flush=!coalesceFlush||r.polls%64==0;
        if(flush)++r.flushes;
        r.status=(long)poll(flush);++r.polls;
        if(r.status!=1)return r; // S_OK or a real error, NOT S_FALSE
        const double elapsed=elapsedMs();
        if(elapsed>100){r.timeout=true;return r;}
        if(cooperative&&elapsed>=.1){++r.yields;if(yield())++r.switched;else relax();}
        else relax();
    }
}
}
