#pragma once
#include <cmath>
namespace weapontrace {
// Relocating only Start changes atan2(End-Start), especially near a target.
// Translate the entire finite ray, retaining its spread and original length.
inline bool Translate(float* start,float* end,const float* offset){
    if(!start||!end||!offset)return false;
    float a[3],b[3];
    for(unsigned i=0;i<3;++i){a[i]=start[i]+offset[i];b[i]=end[i]+offset[i];
        if(!std::isfinite(a[i])||!std::isfinite(b[i]))return false;}
    for(unsigned i=0;i<3;++i){start[i]=a[i];end[i]=b[i];}return true;
}
}
