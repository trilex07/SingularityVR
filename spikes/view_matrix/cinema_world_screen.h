#pragma once
#include <cmath>
#include <initializer_list>

namespace cinemascreen {
struct Pose { float x=0,y=0,z=0,qx=0,qy=0,qz=0,qw=1; };
inline Pose Centre(const Pose& l,const Pose& r) {
    const float sign=l.qx*r.qx+l.qy*r.qy+l.qz*r.qz+l.qw*r.qw<0?-1.f:1.f;
    return {(l.x+r.x)*.5f,(l.y+r.y)*.5f,(l.z+r.z)*.5f,
        (l.qx+sign*r.qx)*.5f,(l.qy+sign*r.qy)*.5f,
        (l.qz+sign*r.qz)*.5f,(l.qw+sign*r.qw)*.5f};
}
struct Fov { float left,right,down,up; }; // tangent slopes, not radians
inline bool EyePoint(const Pose& eye,float x,float y,float z,float (&out)[3]) {
    const float n=eye.qx*eye.qx+eye.qy*eye.qy+eye.qz*eye.qz+eye.qw*eye.qw;
    if(!std::isfinite(n)||n<.25f||n>4.f)return false;
    const float inv=1.f/std::sqrt(n),qx=-eye.qx*inv,qy=-eye.qy*inv,qz=-eye.qz*inv,qw=eye.qw*inv;
    x-=eye.x;y-=eye.y;z-=eye.z;
    const float tx=2*(qy*z-qz*y),ty=2*(qz*x-qx*z),tz=2*(qx*y-qy*x);
    out[0]=x+qw*tx+qy*tz-qz*ty;out[1]=y+qw*ty+qz*tx-qx*tz;
    out[2]=z+qw*tz+qx*ty-qy*tx;
    return std::isfinite(out[0])&&std::isfinite(out[1])&&std::isfinite(out[2]);
}
// Keep the cinema FRAME at zero binocular disparity while retaining the
// independently rendered scene images. A shared physical plane instead gives
// its edges IPD/distance disparity, even with exactly equal quad dimensions.
// Translation only: preserve the world anchor orientation and head-motion pan.
inline bool BinocularFramePose(const Pose& anchor,const Pose (&eyes)[2],unsigned eye,Pose& out) {
    if(eye>1)return false;
    const float dx=eyes[1].x-eyes[0].x,dy=eyes[1].y-eyes[0].y,dz=eyes[1].z-eyes[0].z;
    const float separation=std::sqrt(dx*dx+dy*dy+dz*dz);
    if(!std::isfinite(separation)||separation<.035f||separation>.09f)return false;
    Pose staged=anchor;
    staged.x+=eyes[eye].x-(eyes[0].x+eyes[1].x)*.5f;
    staged.y+=eyes[eye].y-(eyes[0].y+eyes[1].y)*.5f;
    staged.z+=eyes[eye].z-(eyes[0].z+eyes[1].z)*.5f;
    if(!std::isfinite(staged.x)||!std::isfinite(staged.y)||!std::isfinite(staged.z))return false;
    out=staged;return true;
}
struct Anchor {
    Pose pose{};
    float width=0, height=0;
    bool valid=false;
    bool stereoFitted=false;
    void Reset() { pose=Pose{};width=height=0;valid=false;stereoFitted=false; }
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
    // One shared physical rectangle, fitted against BOTH actual eye frusta.
    // The old left-eye-only width ignored vertical/asymmetric FOV and IPD.
    bool LatchStereo(const Pose& head,float distance,float halfTanX,float fraction,float aspect,
                     const Pose (&eyes)[2],const Fov (&fovs)[2]) {
        if(valid && stereoFitted)return true; // mono bootstrap is NOT a stereo fit
        Anchor candidate;if(!candidate.Latch(head,distance,halfTanX,fraction,aspect))return false;
        float fit=1.f;
        const float yaw=2*std::atan2(candidate.pose.qy,candidate.pose.qw);
        const float rx=std::cos(yaw),rz=-std::sin(yaw);
        for(unsigned e=0;e<2;++e){
            const auto f=fovs[e];
            if(!std::isfinite(f.left)||!std::isfinite(f.right)||!std::isfinite(f.down)||
               !std::isfinite(f.up)||f.left>=0||f.right<=0||f.down>=0||f.up<=0)return false;
            float c[3];if(!EyePoint(eyes[e],candidate.pose.x,candidate.pose.y,candidate.pose.z,c)||c[2]>=0)return false;
            for(int sx:{-1,1})for(int sy:{-1,1}){
                float corner[3];
                if(!EyePoint(eyes[e],candidate.pose.x+sx*candidate.width*.5f*rx,
                    candidate.pose.y+sy*candidate.height*.5f,
                    candidate.pose.z+sx*candidate.width*.5f*rz,corner))return false;
                const float v[3]={corner[0]-c[0],corner[1]-c[1],corner[2]-c[2]};
                // clip inequalities b + a*scale >= 0, including eye near-plane
                const float b[5]={c[0]+f.left*c[2],-c[0]-f.right*c[2],
                    c[1]+f.down*c[2],-c[1]-f.up*c[2],-c[2]};
                const float a[5]={v[0]+f.left*v[2],-v[0]-f.right*v[2],
                    v[1]+f.down*v[2],-v[1]-f.up*v[2],-v[2]};
                for(unsigned i=0;i<5;++i){if(b[i]<=0)return false;
                    if(a[i]<0)fit=std::fmin(fit,b[i]/-a[i]);}
            }
        }
        if(!std::isfinite(fit)||fit<=0)return false;
        candidate.width*=fit;candidate.height*=fit;candidate.stereoFitted=true;*this=candidate;return true;
    }
};
inline int ClampDistanceMm(int mm) {return mm<800?800:mm>10000?10000:mm;}
inline bool TrackingAllowsAnchor(bool alreadyAnchored,bool validPositions) {
    return alreadyAnchored || validPositions;
}
}
