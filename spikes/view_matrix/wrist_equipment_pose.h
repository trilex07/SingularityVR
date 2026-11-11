#pragma once
#include "weapon_reload_carry.h"
namespace wristhud {
// Native-local bind units -> final skin -> actual section-local -> actual
// camera/controller ride -> inverse UE-world/tracking map. Exactly the same
// final ride must be supplied as StereoPair uses for this submitted section.
inline bool FinalWrist(const armbind::Mat& bind,const float skin[3][4],
                       const armbind::Mat& local,const armbind::Mat& ride,
                       const armbind::Mat& trackingMap,armbind::Mat& out){
    if(!armbind::Finite(local)||!armbind::Finite(ride))return false;
    armbind::Mat chain;skeletal::Multiply(chain.m,local.m,ride.m);
    return weaponcarry::WristTracking(bind,skin,chain,trackingMap,out);
}
}
