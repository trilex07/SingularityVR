#pragma once
#include "seeker_geometry.h"
#include "weapon_trace_math.h"
#include "hip_knife.h"
namespace seekerprojectile {
struct Launch {bool valid=false;uint32_t pawn=0,weapon=0;hipknife::V origin{},axis{};};
inline bool Build(const seekergeometry::Witness& w,uint32_t pawn,uint32_t weapon,uint64_t now,
                  const armbind::Mat& controller,hipknife::V camera,Launch& out){
    if(!w.Current(pawn,weapon,now)||!handanchor::Affine(controller.m)||!hipknife::Finite(camera))return false;
    armbind::Mat inverse;
    if(!skeletal::Inverse(inverse.m,seekergeometry::RootBind.m))return false;
    const auto chain=seekergeometry::Product(seekergeometry::Product(inverse,w.rootLocal),controller);
    const auto p=handanchor::Point(seekergeometry::Muzzle,chain.m);
    const auto q=handanchor::Point(seekergeometry::Muzzle+seekergeometry::Forward,chain.m);
    armik::V unit;if(!armik::Unit(q-p,unit)||!armik::Finite(p))return false;
    out={true,pawn,weapon,{camera.x+p.x,camera.y+p.y,camera.z+p.z},{unit.x,unit.y,unit.z}};
    return true;
}
inline bool Spread(const Launch& l,uint32_t weapon,int pitch,int yaw,int* native){
    if(!l.valid||l.weapon!=weapon||!native)return false;
    const auto direction=[](int p,int y,float out[3]){
        const double k=6.283185307179586/65536.;
        const double pp=p*k,yy=y*k,c=std::cos(pp);
        out[0]=(float)(c*std::cos(yy));out[1]=(float)(c*std::sin(yy));out[2]=(float)std::sin(pp);
    };
    float centre[3],end[3],start[3]{};direction(pitch,yaw,centre);direction(native[0],native[1],end);
    const float axis[]{l.axis.x,l.axis.y,l.axis.z};
    if(!weapontrace::Align(start,end,centre,axis))return false;
    constexpr double rot=65536./6.283185307179586;
    native[0]=(int)std::lround(std::atan2(end[2],std::hypot(end[0],end[1]))*rot);
    native[1]=(int)std::lround(std::atan2(end[1],end[0])*rot);
    // Native spread is rotated, not removed; roll/recoil/projectile flight intact.
    return true;
}
}
