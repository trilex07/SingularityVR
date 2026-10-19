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
inline bool Position(const weaponreloadanim::Data& data,const armbind::Mat& skin,
                     const armbind::Mat& local,const armbind::Mat& controller,hipknife::V& out){
    hipknife::V p;
    if(!data.ready||!data.nativeSpace||!RootPoint(data.kind,p)||!armbind::Finite(data.rootBind)||
       !armbind::Finite(skin)||!armbind::Finite(local)||!armbind::Finite(controller))return false;
    p=weaponreload::Transform(weaponreload::Transform(weaponreload::Transform(
        weaponreload::Transform(p,data.rootBind),skin),local),controller);
    if(!hipknife::Finite(p))return false;out=p;return true;
}
struct Witness {
    bool valid=false;weaponreload::Kind kind=weaponreload::Kind::None;
    std::uint32_t pawn=0,weapon=0;std::uint64_t tick=0;hipknife::V relative{};
    bool Current(std::uint32_t p,std::uint32_t w,std::uint64_t now)const{
        return valid&&p&&w&&p==pawn&&w==weapon&&now>=tick&&now-tick<=150&&hipknife::Finite(relative);
    }
};
}
