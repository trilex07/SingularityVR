#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace wristhud {
// VrModeOn is the draw-duplication flag itself. Never reject duplication
// separately after requiring VR: doing so makes preview unreachable.
inline bool RuntimeAllowed(bool stereo,bool eyes,bool paused,bool menu,bool panel,
                           bool cinematic,bool support,bool gameplay){
    return stereo&&eyes&&!paused&&!menu&&!panel&&!cinematic&&!support&&gameplay;
}
struct Values {
    bool health=false,energy=false,tech=false,packs=false,vials=false;
    float hp=0,maxHp=0,tmd=0,maxTmd=0;
    int e99Tech=0,healthPacks=0,e99Vials=0;
};
inline bool ValidHealth(float current,float maximum){
    return std::isfinite(current)&&std::isfinite(maximum)&&maximum>0&&
           current>=0&&current<=maximum*2;
}
inline bool ValidEnergy(float current,float maximum){
    return std::isfinite(current)&&std::isfinite(maximum)&&maximum>0&&
           current>=0&&current<=maximum*2;
}
inline bool ValidCount(int value){return value>=0&&value<=1000000;}
inline int Cells(float current,float maximum,int slots=10){
    if(!ValidHealth(current,maximum)||slots<=0)return -1;
    return std::clamp((int)std::ceil(std::clamp(current/maximum,0.f,1.f)*slots),0,slots);
}
inline bool Current(std::uint64_t sampled,std::uint64_t now,std::uint64_t maxAge=150){
    return sampled&&now>=sampled&&now-sampled<=maxAge;
}
inline bool Visibility(bool shown,float facing,float distance,float heightBelowEye,bool tracked,
                       bool gameplay,bool paused,bool menu,bool cinematic,bool support){
    if(!tracked||!gameplay||paused||menu||cinematic||support||
       !std::isfinite(facing)||!std::isfinite(distance)||!std::isfinite(heightBelowEye))return false;
    const float dot=shown?.16f:.34f;
    const float maxDistance=shown?.72f:.62f;
    return facing>=dot&&distance>=.16f&&distance<=maxDistance&&heightBelowEye<=.50f;
}
inline bool SuppressLegacy(bool enabled,bool leftEyeDrawn,bool rightEyeDrawn,bool healthValid){
    return enabled&&leftEyeDrawn&&rightEyeDrawn&&healthValid;
}
} // namespace wristhud
