#pragma once
#include "weapon_reload_runtime.h"
namespace weaponmuzzle {
// Named muzzle_flash relative to named weapon cog from ORIGINAL PSKs. These
// are bone-local UU, not controller offsets. Verify via audit_weapon_mounts.py.
inline bool RootPoint(weaponreload::Kind kind,hipknife::V& out){
    using K=weaponreload::Kind;
    if(kind==K::Valkyrie)out={0,-3.49137115f,36.9269905f};
    else if(kind==K::Volk)out={-.000018f,-5.37408447f,61.2917137f};
    else if(kind==K::Kasimov)out={-.000018f,-3.84281158f,67.725647f};
    else return false;return true;
}
inline bool RootPosition(const weaponreloadanim::Data& data,const armbind::Mat& skin,
                     const armbind::Mat& local,const armbind::Mat& controller,hipknife::V p,hipknife::V& out){
    if(!data.ready||!data.nativeSpace||!hipknife::Finite(p)||!armbind::Finite(data.rootBind)||
       !armbind::Finite(skin)||!armbind::Finite(local)||!armbind::Finite(controller))return false;
    p=weaponreload::Transform(weaponreload::Transform(weaponreload::Transform(
        weaponreload::Transform(p,data.rootBind),skin),local),controller);
    if(!hipknife::Finite(p))return false;out=p;return true;
}
inline bool Position(const weaponreloadanim::Data& data,const armbind::Mat& skin,
                     const armbind::Mat& local,const armbind::Mat& controller,hipknife::V& out){
    hipknife::V p;return RootPoint(data.kind,p)&&RootPosition(data,skin,local,controller,p,out);
}
inline bool ToTracking(hipknife::V p,hipknife::V centre,float yaw,float base,float units,hipknife::V& out){
    if(!hipknife::Finite(p)||!hipknife::Finite(centre)||!std::isfinite(yaw)||!std::isfinite(base)||
       !std::isfinite(units)||units<10||units>1000)return false;
    const float x=p.x/units,y=p.y/units,z=p.z/units;
    const float f=x*std::cos(base)+y*std::sin(base),r=-x*std::sin(base)+y*std::cos(base);
    out={centre.x-std::sin(yaw)*f+std::cos(yaw)*r,centre.y+z,
        centre.z-std::cos(yaw)*f-std::sin(yaw)*r};return hipknife::Finite(out);
}
struct Witness {
    bool valid=false;weaponreload::Kind kind=weaponreload::Kind::None;
    std::uint32_t pawn=0,weapon=0;std::uint64_t tick=0;hipknife::V relative{};
    hipknife::V forward{}; // exact rendered root +Z, native world axes
    bool Current(std::uint32_t p,std::uint32_t w,std::uint64_t now)const{
        return valid&&p&&w&&p==pawn&&w==weapon&&now>=tick&&now-tick<=150&&hipknife::Finite(relative);
    }
    bool Direction(std::uint32_t p,std::uint32_t w,std::uint64_t now,hipknife::V& out)const{
        const float n=hipknife::Length(forward);
        if(!Current(p,w,now)||!hipknife::Finite(forward)||!std::isfinite(n)||std::fabs(n-1.f)>.001f)return false;
        out=forward;return true;
    }
};
}
