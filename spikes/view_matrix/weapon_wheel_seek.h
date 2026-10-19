#pragma once
#include <cstdint>
// Follow the native cycle one observed transition at a time. Catalog order is
// a proposal, not inventory evidence; never queue several blind cycle pulses.
namespace wheelseek {
enum class Result { Wait, Pulse, Landed, Failed };
struct State {
    long target=0,last=0,visited[8]{};
    unsigned count=0;
    uint64_t deadline=0;
};
inline bool Begin(State& s,long current,long target,uint64_t now) {
    s={};
    if(current<=0||target<=0||current==target)return false;
    s.target=target;s.last=current;s.visited[s.count++]=current;s.deadline=now+5000;
    return true;
}
inline Result Observe(State& s,long current,uint64_t now,bool valid) {
    if(!s.target)return Result::Wait;
    if(!valid||now>=s.deadline){s={};return Result::Failed;}
    if(current==s.target){s={};return Result::Landed;}
    if(current<=0||current==s.last)return Result::Wait;
    for(unsigned i=0;i<s.count;++i)if(s.visited[i]==current){s={};return Result::Failed;}
    if(s.count>=8){s={};return Result::Failed;}
    s.visited[s.count++]=current;s.last=current;
    return Result::Pulse;
}
}
