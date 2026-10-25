#pragma once
#include "two_hand_grip.h"
#include <cstdint>

// Read-only XR-thread diagnostic. Never supplies an acquisition decision, changes
// a latch or substitutes a controller pose. At most three receipts per squeeze,
// for 24 squeezes total; holding Grip does not produce frame-by-frame logging.
namespace supportprobe {
struct Sample {
    std::uint32_t frame=0, pawn=0, weapon=0, vertices=0;
    bool raw=false, effective=false, tracked=false, weaponContext=false;
    bool enabled=false, gameplay=false, presented=false, holstered=false, paused=false, menu=false;
    bool knife=false, reload=false, cylinder=false, pistol=false, closeLongGun=false, acquire=false, active=false;
    int hands=0, aimHand=1, offHand=0;
    float delta[3]{}, forward[3]{}, primary[3]{}, support[3]{};
    float Distance() const {return std::sqrt(delta[0]*delta[0]+delta[1]*delta[1]+delta[2]*delta[2]);}
    float Alignment() const {
        const float d=Distance();
        return d>0.f ? (delta[0]*forward[0]+delta[1]*forward[1]+delta[2]*forward[2])/d : 0.f;
    }
};
inline const char* Gate(const Sample& s) {
    if(!s.raw)return "released";
    if(!s.effective)return s.knife?"knife-owned":s.reload?"reload-owned":s.cylinder?"cylinder-active":"grip-reserved";
    if(!s.weaponContext) {
        if(!s.enabled)return "disabled";
        if(!s.gameplay)return "not-gameplay";
        if(!s.presented)return "weapon-not-presented";
        if(s.holstered)return "holstered";
        if(s.paused)return "paused";
        if(s.menu)return "vr-menu";
        return "context-changed";
    }
    if(s.active)return "latched";
    if(!s.tracked)return "tracking";
    if(s.acquire)return "eligible-unlatched"; // Includes the latch's one-sample identity reset.
    const float d=s.Distance();
    if(!std::isfinite(d))return "invalid-distance";
    if(d<((s.pistol||s.closeLongGun)?.045f:.12f))return "too-close";
    if(d>(s.pistol?.38f:.50f))return "too-far";
    // Gate thresholds are DIAGNOSTIC labels only. Production's acquire result
    // above stays authoritative and is also printed, preventing silent drift.
    if(!(s.Alignment()>(s.pistol?-.35f:(s.closeLongGun&&d<=.30f?-.75f:.50f))))return "forward-cone";
    return "geometry-unclassified";
}
enum class Event { None, Press, Acquired, Release };
inline const char* Name(Event e){return e==Event::Press?"press":e==Event::Acquired?"acquired":"release-last-held";}
struct Receipt {Event event=Event::None; Sample sample{}; unsigned attempt=0;};
class Budget {
public:
    static constexpr unsigned MaxAttempts=24;
    Receipt Observe(bool enabled,const Sample& s) {
        if(!enabled){held_=s.raw;have_=false;return {};}
        const bool press=s.raw&&!held_,release=!s.raw&&held_;
        held_=s.raw;
        if(press){
            have_=attempts_<MaxAttempts;
            if(!have_)return {};
            ++attempts_;last_=s;acquired_=s.active;
            return {Event::Press,s,attempts_};
        }
        if(release&&have_){have_=false;return {Event::Release,last_,attempts_};}
        if(s.raw&&have_){
            last_=s;
            if(s.active&&!acquired_){acquired_=true;return {Event::Acquired,s,attempts_};}
        }
        return {};
    }
private:
    Sample last_{};unsigned attempts_=0;bool held_=false,have_=false,acquired_=false;
};
}
