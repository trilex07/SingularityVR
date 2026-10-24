#pragma once
#include <cstdint>

// Read-only, render-thread-only acquisition budget. Initial pose + action/state
// changes, each sampled on arrival and after 250ms unchanged. Never drains the
// session budget just because the player takes time to reach gameplay.
namespace dualpose {
struct State {
    unsigned hands=0, support=0, actions=0, gun=0;
    std::uintptr_t weapon=0;
    bool operator==(const State& b) const {
        return hands==b.hands && support==b.support && actions==b.actions &&
            gun==b.gun && weapon==b.weapon;
    }
};
class Budget {
public:
    static constexpr unsigned maxTransitions=16, settleMs=250;
    bool Observe(bool enabled, unsigned frame, std::uint32_t tick, const State& state) {
        if(!enabled)return false;
        if(haveFrame_ && frame==frame_)return active_;
        haveFrame_=true;frame_=frame;active_=false;
        if(!transitions_ || !(state==state_)) {
            if(transitions_==maxTransitions)return false;
            state_=state;since_=tick;++transitions_;settled_=false;active_=true;phase_=0;
        } else if(!settled_ && std::uint32_t(tick-since_)>=settleMs) {
            settled_=true;active_=true;phase_=1;
        }
        seen_=0;
        return active_;
    }
    bool Claim(unsigned frame,unsigned part,unsigned eye) {
        if(!active_||frame!=frame_||part>=3||eye>=2)return false;
        const auto bit=1u<<(part*2+eye);
        if(seen_&bit)return false;
        seen_|=bit;return true;
    }
    void EndFrame(){active_=false;}
    bool Active()const{return active_;}
    unsigned Frame()const{return frame_;}
    unsigned Transition()const{return transitions_;}
    unsigned Phase()const{return phase_;}
    const State& Current()const{return state_;}
private:
    State state_{};unsigned frame_=0,transitions_=0,phase_=0,seen_=0;
    std::uint32_t since_=0;bool haveFrame_=false,active_=false,settled_=false;
};
}
