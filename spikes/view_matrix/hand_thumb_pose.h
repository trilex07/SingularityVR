#pragma once
#include "glove_animation.h"

namespace thumbpose {
// Bounded adaptive fist path + knife x left/right x three ORIGINAL thumb joints. Geometry/mount and
// non-thumb rig stay in their existing files. This optional cache is atomic.
struct Data {unsigned count=0;float times[65]{};gloves::Q q[66][2][3]{};};
inline bool Decode(const unsigned char* p,std::size_t n,Data& out){
    if(!p||n<16||std::memcmp(p,"SVNT",4)||gloves::U32(p+4)!=3||gloves::U32(p+12)!=3)return false;
    const unsigned count=gloves::U32(p+8);
    if(count<2||count>65||n!=16+count*4+(count+1)*96)return false;
    Data next;next.count=count;std::memcpy(next.times,p+16,count*4);std::memcpy(next.q,p+16+count*4,(count+1)*96);
    if(next.times[0]!=0||next.times[count-1]!=1)return false;
    for(unsigned i=0;i<count;++i)if(!std::isfinite(next.times[i])||next.times[i]<0||next.times[i]>1||(i&&next.times[i]<=next.times[i-1]))return false;
    for(unsigned mode=0;mode<=count;++mode)for(const auto& hand:next.q[mode])for(const auto& q:hand){
        gloves::Q normalized;
        if(!gloves::Normalize(q,normalized)||q.w<0||
           std::fabs(q.x-normalized.x)+std::fabs(q.y-normalized.y)+
           std::fabs(q.z-normalized.z)+std::fabs(q.w-normalized.w)>.002f)return false;
        if(!mode&&std::fabs(q.x)+std::fabs(q.y)+std::fabs(q.z)+std::fabs(q.w-1)>1e-5f)return false;
    }
    out=next;return true;
}
inline bool Thumb(unsigned b){return (b>=10&&b<=12)||(b>=70&&b<=72);}
inline unsigned Segment(unsigned b){return b>=70?b-70:b-10;}
inline bool Sample(const Data& data,unsigned side,unsigned segment,float grip,bool knife,gloves::Q& out){
    if(side>1||segment>2||!std::isfinite(grip)||grip<0||grip>1)return false;
    if(data.count<2||data.count>65)return false;
    if(knife){out=data.q[data.count][side][segment];return true;}
    unsigned lo=0;while(lo+2<data.count&&data.times[lo+1]<grip)++lo;
    const float t=(grip-data.times[lo])/(data.times[lo+1]-data.times[lo]);
    const gloves::Q a=data.q[lo][side][segment];auto b=data.q[lo+1][side][segment];
    if(a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w<0)b={-b.x,-b.y,-b.z,-b.w};
    return gloves::Normalize({a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t,a.w+(b.w-a.w)*t},out);
}
}
