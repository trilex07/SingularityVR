#pragma once
#include "centurion_reload.h"

// Presentation-only calibration. The original cylinder remains on the native
// gun. No animation palette, engine socket or inventory operation is inferred.
namespace centurionclip {
using hipknife::Pose;using hipknife::V;using hipknife::Q;
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
