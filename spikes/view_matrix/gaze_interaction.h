#pragma once
#include <cmath>
#include <cstdint>
#include "psvr2_gaze_share.h"
#include "gaze_fovea.h"
#include "weapon_wheel.h"

namespace svrgaze {
inline bool Direction(const svr::psvr2::GazeSample& s,uint64_t now,float (&d)[3]) {
    using namespace svr::psvr2;
    if(now<s.publishMs || now-s.publishMs>100 || !(s.flags&GazeCombinedValid) ||
       ((s.flags&GazeBlinkLeft)&&(s.flags&GazeBlinkRight)))return false;
    // Same measured Toolkit -> OpenXR VIEW conversion as Darkness2VR.
    d[0]=-s.dirCombined[0];d[1]=s.dirCombined[1];d[2]=-s.dirCombined[2];
    const float n=std::sqrt(d[0]*d[0]+d[1]*d[1]+d[2]*d[2]);
    if(!std::isfinite(n)||n<.5f||n>1.5f||d[2]>=-.001f)return false;
    for(float& x:d)x/=n;
    return true;
}
// Coordinates come from the actual displayed eye-0 wheel, not a second layout.
inline int Sector(float px,float py,float cx,float cy,float inner,float outer,int slots) {
    const float x=px-cx,y=py-cy,r2=x*x+y*y;
    if(!std::isfinite(r2)||r2<inner*inner||r2>outer*outer)return -1;
    return weaponwheel::SectorForStick(x,y,slots,0.f);
}
struct Dwell {
    int candidate=-1;uint64_t since=0;
    void Reset(){candidate=-1;since=0;}
    int Update(int sector,uint64_t now){
        if(sector<0){Reset();return -1;}
        if(candidate!=sector||now<since){candidate=sector;since=now;return -1;}
        return now-since>=80?sector:-1;
    }
};
}
