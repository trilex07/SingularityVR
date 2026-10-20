#pragma once
#include "sniper_scope.h"
#include "ui_pointer.h"
namespace scopepip {
using hipknife::V;
struct Frame {
    bool active=false;std::uint32_t pawn=0,weapon=0;
    std::uint64_t tick=0;V lens{},forward{},up{};float radius=0;int eye=-1;
    bool Current(std::uint32_t p,std::uint32_t w,std::uint64_t now)const{
        return active&&p&&w&&pawn==p&&weapon==w&&now>=tick&&now-tick<=150&&
            hipknife::Finite(lens)&&hipknife::Finite(forward)&&hipknife::Finite(up)&&
            radius>.003f&&radius<.06f&&eye>=0&&eye<2;
    }
};
inline V Cross(V a,V b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline bool Unit(V p,V& result){const float n=hipknife::Length(p);
    if(!hipknife::Finite(p)||!std::isfinite(n)||n<.0001f)return false;result=p*(1.f/n);return true;}
inline bool Basis(V forward,V up,V& right,V& vertical){
    V f;if(!scopepip::Unit(forward,f)||!scopepip::Unit(scopepip::Cross(f,up),right)||!scopepip::Unit(scopepip::Cross(right,f),vertical))return false;
    return true;
}
inline bool Angles(V forward,V up,int& pitch,int& yaw,int& roll){
    V f{},right{},vertical{};
    if(!scopepip::Unit(forward,f)||!scopepip::Basis(f,up,right,vertical))return false;
    const float y=std::atan2(f.y,f.x),scale=65536.f/6.2831853f;
    // Analytic zero-roll right avoids cross(forward, worldUp)'s singularity
    // when aiming straight up/down. Matches that basis away from the pole.
    const V zeroRight{std::sin(y),-std::cos(y),0};
    V zeroUp{};if(!scopepip::Unit(scopepip::Cross(zeroRight,f),zeroUp))return false;
    pitch=(int)(std::atan2(f.z,std::sqrt(f.x*f.x+f.y*f.y))*scale);
    yaw=(int)(y*scale);
    roll=(int)(std::atan2(hipknife::Dot(vertical,zeroRight),hipknife::Dot(vertical,zeroUp))*scale);
    return true;
}
// Exact inverse of weaponmuzzle::ToTracking, applied to a position relative to
// the current native camera. This is NOT a crop of an HMD eye image.
inline bool Relative(V tracking,V centre,float yaw,float base,float units,V& out){
    if(!hipknife::Finite(tracking)||!hipknife::Finite(centre)||!std::isfinite(yaw)||
       !std::isfinite(base)||!std::isfinite(units)||units<10||units>1000)return false;
    const V d=tracking-centre;
    const float f=-std::sin(yaw)*d.x-std::cos(yaw)*d.z;
    const float r= std::cos(yaw)*d.x-std::sin(yaw)*d.z;
    out={(std::cos(base)*f-std::sin(base)*r)*units,
         (std::sin(base)*f+std::cos(base)*r)*units,d.y*units};
    return hipknife::Finite(out);
}
struct Completion {
    bool bound=false;std::uint32_t draws=0,epoch=0;
    void Reset(){bound=false;draws=epoch=0;}
    void Bind(bool target){if(bound&&!target&&draws){++epoch;draws=0;}bound=target;}
    void Draw(bool succeeded){if(bound&&succeeded&&draws<0xffffffffu)++draws;}
};
// Bound expensive native rendering when completed content cannot be presented.
// Only eligible, completed frames count: menus/missing eye poses do not trip it.
struct PresentationBudget {
    unsigned missed=0;
    bool fault=false;
    bool Observe(bool complete,bool presented){
        if(fault)return false;
        if(presented)missed=0;
        else if(complete&&++missed>=30)fault=true;
        return !fault;
    }
};
// Project directly from tracking poses, never from a camera matrix last seen
// during an unrelated UE3 draw (which may belong to the optical capture).
struct Pixel { float x=0,y=0,rhw=0; };
struct RenderLease {
    unsigned& depth;bool own;
    RenderLease(unsigned& d,bool enabled,std::uintptr_t expected,std::uintptr_t actual):
        depth(d),own(enabled&&expected&&expected==actual){if(own)++depth;}
    ~RenderLease(){if(own)--depth;}
    RenderLease(const RenderLease&)=delete;
    RenderLease& operator=(const RenderLease&)=delete;
};
inline bool EyePixel(unsigned eye,unsigned width,unsigned height,
                     const uipointer::ScreenPoint& p,Pixel& out){
    if(eye>1||width<4||(width&1)||!height||width>32768||height>32768||
       !std::isfinite(p.u)||!std::isfinite(p.v)||!std::isfinite(p.depth)||p.depth<=1e-5f)return false;
    const Pixel result{(eye+p.u)*(width*.5f),p.v*height,1.f/p.depth};
    if(!std::isfinite(result.x)||!std::isfinite(result.y)||!std::isfinite(result.rhw))return false;
    out=result;return true;
}
}
