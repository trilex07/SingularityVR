#pragma once
#include <cmath>
#include <cstdint>

namespace reloadcontrols {
// Semantic remap of existing XR face actions; no additional action polling or
// change to native menu select/back. Gameplay-only, independently default OFF.
struct Result {bool health=false,releaseCylinder=false,jump=false;};
enum class Face {Other,Cross,Circle};
struct FaceOutput {bool down;std::uint16_t mask;bool health;};
inline FaceOutput RouteFace(bool layout,bool gameplay,Face face,bool raw,
                           std::uint16_t mask,bool legacyHealth,const Result& a){
    if(!layout||!gameplay)return {raw,mask,legacyHealth}; // exact menu/legacy behavior
    if(face==Face::Cross)return {a.health,0x0001,true}; // native Health, not native A/Jump
    if(face==Face::Circle)return {false,0,false}; // semantic cylinder action, no game bit
    return {raw,mask,legacyHealth};
}
struct State {
    bool crossBlocked=true,circlePrior=true,jumpArmed=false;
    std::uint64_t jumpUntil=0,lastMs=0;
    Result Update(bool context,bool cross,bool circle,float x,float y,std::uint64_t now){
        const bool edge=circle&&!circlePrior;circlePrior=circle;
        if(!context||!std::isfinite(x)||!std::isfinite(y)||now<lastMs){
            crossBlocked=true;circlePrior=true;jumpArmed=false;jumpUntil=0;lastMs=now;return {};
        }
        lastMs=now;
        if(!cross)crossBlocked=false;
        if(std::fabs(x)<.35f&&std::fabs(y)<.35f)jumpArmed=true;
        // Dominant up gesture, not a diagonal snap-turn. Any strong deflection
        // consumes arming; centring is mandatory before another jump.
        if(jumpArmed&&(std::fabs(x)>.75f||std::fabs(y)>.75f)){
            jumpArmed=false;
            if(y>.75f&&std::fabs(x)<.45f)jumpUntil=now+120;
        }
        return {cross&&!crossBlocked,edge,now<jumpUntil};
    }
};
} // namespace reloadcontrols
