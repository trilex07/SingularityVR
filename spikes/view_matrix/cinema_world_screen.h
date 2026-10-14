#pragma once
#include <cmath>

namespace cinemascreen {
struct Pose { float x=0,y=0,z=0,qx=0,qy=0,qz=0,qw=1; };
inline Pose Centre(const Pose& l,const Pose& r) {
    const float sign=l.qx*r.qx+l.qy*r.qy+l.qz*r.qz+l.qw*r.qw<0?-1.f:1.f;
    return {(l.x+r.x)*.5f,(l.y+r.y)*.5f,(l.z+r.z)*.5f,
        (l.qx+sign*r.qx)*.5f,(l.qy+sign*r.qy)*.5f,
        (l.qz+sign*r.qz)*.5f,(l.qw+sign*r.qw)*.5f};
}
struct Anchor {
    Pose pose{};
    float width=0, height=0;
    bool valid=false;
    void Reset() { pose=Pose{};width=height=0;valid=false; }
    // Pausing a cinematic does not end it: do not re-anchor on menu return.
    void Observe(bool cinematic, bool enabled) { if(!cinematic || !enabled)Reset(); }
    bool Latch(const Pose& head,float distance,float halfTanX,float widthFraction,float aspect) {
        if(valid)return true;
        const float values[]={head.x,head.y,head.z,head.qx,head.qy,head.qz,head.qw,
                              distance,halfTanX,widthFraction,aspect};
        for(float v:values)if(!std::isfinite(v))return false;
        if(distance<.8f || distance>10.f || halfTanX<=0 || halfTanX>4.f ||
           widthFraction<=0 || widthFraction>1.f || aspect<=.1f || aspect>10.f)return false;
        const float norm=head.qx*head.qx+head.qy*head.qy+head.qz*head.qz+head.qw*head.qw;
        if(norm<.25f || norm>4.f)return false;
        const float s=1.f/std::sqrt(norm);
        const float qx=head.qx*s,qy=head.qy*s,qz=head.qz*s,qw=head.qw*s;
        float fx=-2.f*(qx*qz+qw*qy),fz=-(1.f-2.f*(qx*qx+qy*qy));
        const float horizontal=std::sqrt(fx*fx+fz*fz);
        if(horizontal<.05f)return false; // no arbitrary heading when looking straight up/down
        fx/=horizontal;fz/=horizontal;
        const float yaw=std::atan2(-fx,-fz);
        Pose staged{head.x+distance*fx,head.y,head.z+distance*fz,
                    0,std::sin(yaw*.5f),0,std::cos(yaw*.5f)};
        const float w=2.f*distance*halfTanX*widthFraction,h=w/aspect;
        if(!std::isfinite(w) || !std::isfinite(h) || w<=0 || h<=0 || w>40.f || h>40.f)return false;
        pose=staged;width=w;height=h;valid=true;return true;
    }
};
inline int ClampDistanceMm(int mm) {return mm<800?800:mm>10000?10000:mm;}
inline bool TrackingAllowsAnchor(bool alreadyAnchored,bool validPositions) {
    return alreadyAnchored || validPositions;
}
}
