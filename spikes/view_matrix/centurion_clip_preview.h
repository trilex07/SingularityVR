#pragma once
#include "centurion_reload.h"
#include <atomic>

// Presentation-only calibration. The original cylinder remains on the native
// gun. No animation palette, engine socket or inventory operation is inferred.
namespace centurionclip {
using hipknife::Pose;using hipknife::V;using hipknife::Q;
enum class DrawResult {Success,Disabled,CaptureFailed,DrawFailed,RestoreFailed};
// Render-only fault policy. This never reserves a hand or changes reload state.
// Lifecycle code may reset it; normal frames cannot clear a failed restore.
class PresentationSafety {
    std::atomic<bool> fault{false};
    static_assert(std::atomic<bool>::is_always_lock_free,"No lock in render fault latch");
public:
    bool Allowed()const{return !fault.load(std::memory_order_relaxed);}
    void Reset(){fault.store(false,std::memory_order_relaxed);}
    template<class Guard,class Draw> DrawResult Render(Guard& guard,Draw draw){
        if(!Allowed())return DrawResult::Disabled;
        if(!guard.valid){fault.store(true,std::memory_order_relaxed);return DrawResult::CaptureFailed;}
        const bool drawn=draw();
        // Attempt the complete outer restore even when drawing failed midway.
        const bool restored=guard.Restore();
        if(!restored){fault.store(true,std::memory_order_relaxed);return DrawResult::RestoreFailed;}
        return drawn?DrawResult::Success:DrawResult::DrawFailed;
    }
};
struct Config {V offset{0,0,-.08f};Q rotation{};}; // authored aim-space mount
struct Snapshot {
    bool visible=false;
    centurionreload::Phase phase=centurionreload::Phase::Idle;
    Pose hand{};
    std::uint32_t pawn=0,weapon=0;
    std::uint64_t sampledMs=0;
};
inline Q Degrees(float pitch,float yaw,float roll){
    constexpr float halfRadians=3.14159265358979323846f/360.f;
    return hipknife::Multiply(hipknife::Multiply(
        {0,std::sin(yaw*halfRadians),0,std::cos(yaw*halfRadians)},
        {std::sin(pitch*halfRadians),0,0,std::cos(pitch*halfRadians)}),
        {0,0,std::sin(roll*halfRadians),std::cos(roll*halfRadians)});
}
inline bool Current(const Snapshot& s,std::uint32_t pawn,std::uint32_t weapon,std::uint64_t now){
    return s.visible&&pawn&&weapon&&s.pawn==pawn&&s.weapon==weapon&&
        now>=s.sampledMs&&now-s.sampledMs<=100&&
        hipknife::Finite(s.hand.p)&&hipknife::Valid(s.hand.q);
}
inline bool Eligible(const Snapshot& s,bool enabled,std::uint32_t pawn,
                     std::uint32_t weapon,std::uint64_t now){
    return enabled&&s.phase==centurionreload::Phase::Carrying&&Current(s,pawn,weapon,now);
}
inline bool Mount(const Pose& hand,const Config& c,Pose& out){
    if(!hipknife::Finite(hand.p)||!hipknife::Valid(hand.q)||
       !hipknife::Finite(c.offset)||hipknife::Length(c.offset)>.30f||!hipknife::Valid(c.rotation))return false;
    Pose staged{hand.p+hipknife::Rotate(hand.q,c.offset),hipknife::Multiply(hand.q,c.rotation)};
    if(!hipknife::Finite(staged.p)||!hipknife::Valid(staged.q))return false;
    out=staged;return true;
}
} // namespace centurionclip
