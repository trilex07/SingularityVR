#pragma once
#include <cmath>
#include <cstdint>
namespace tmdaim {
struct Pose {
    float matrix[4][4]{},anchor[3]{},camera[3]{},forward[3]{};
    std::uintptr_t pawn=0;
    std::uint64_t time=0;
};
inline bool Current(const Pose& p,std::uintptr_t pawn,std::uint64_t now) {
    return pawn && p.pawn==pawn && p.time && now>=p.time && now-p.time<=150;
}
// Exactly the row-vector transform used by BuildTmdC, not a second Euler/sign path.
// Map its original anchor and view-forward; normalize away uniform mesh scale.
inline bool Ray(const Pose& p,float* origin,float* direction) {
    if(!origin||!direction||origin==direction)return false;
    float o[3]{},d[3]{};
    for(unsigned j=0;j<3;++j) {
        // Finish camera-relative cancellation before adding large world coords.
        o[j]=p.matrix[3][j];
        for(unsigned i=0;i<3;++i) {
            o[j]+=p.anchor[i]*p.matrix[i][j];
            d[j]+=p.forward[i]*p.matrix[i][j];
        }
        o[j]+=p.camera[j];
        if(!std::isfinite(o[j])||!std::isfinite(d[j]))return false;
    }
    const float n=std::sqrt(d[0]*d[0]+d[1]*d[1]+d[2]*d[2]);
    if(!std::isfinite(n)||n<0.00001f)return false;
    for(unsigned j=0;j<3;++j){origin[j]=o[j];direction[j]=d[j]/n;}
    return true;
}
inline bool Eligible(bool enabled,bool gameplay,bool tracked,bool local,bool thread,bool nestedGun) {
    return enabled&&gameplay&&tracked&&local&&thread&&!nestedGun;
}
}
