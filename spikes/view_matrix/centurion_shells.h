#pragma once
#include "centurion_clip_preview.h"
#include "pose_conversion.h"

// Cosmetic original-mesh cases, not a UE3 particle spawn or inventory operation.
// Fixed six-item storage, analytical world-space flight, no collision/sound.
namespace centurionshells {
using hipknife::V;using hipknife::Q;using hipknife::Pose;
inline bool NeedsPass(bool warmed,bool markers,bool clip,unsigned cases){return !warmed||markers||clip||cases!=0;}
struct WorldFrame {
    V origin{},centre{}; // existing camera world UU / tracking recenter metres
    float centreYaw=0,baseYaw=0,units=0;
};
inline bool Valid(const WorldFrame& f){return hipknife::Finite(f.origin)&&hipknife::Finite(f.centre)&&
    std::isfinite(f.centreYaw)&&std::isfinite(f.baseYaw)&&std::isfinite(f.units)&&f.units>=10&&f.units<=1000;}
inline V WorldDelta(V v,const WorldFrame& f){
    const auto p=vrpose::TrackingDeltaToWorldUU({v.x,v.y,v.z},f.centreYaw,f.baseYaw,f.units);
    return {p.x,p.y,p.z};
}
inline V ToWorld(V p,const WorldFrame& f){return f.origin+WorldDelta(p-f.centre,f);}
inline V ToTracking(V p,const WorldFrame& f){
    p=(p-f.origin)*(1.f/f.units);
    const float bs=std::sin(f.baseYaw),bc=std::cos(f.baseYaw),sc=std::sin(f.centreYaw),cc=std::cos(f.centreYaw);
    const float forward=bc*p.x+bs*p.y,right=-bs*p.x+bc*p.y;
    return f.centre+V{-sc*forward+cc*right,p.z,-cc*forward-sc*right};
}
struct Case {V origin{},velocity{};Q orientation{};};
struct Burst {
    Case cases[6]{};
    std::uint32_t pawn=0,weapon=0;
    std::uint64_t born=0;
    unsigned count=0;
    float yaw=0,units=0;
};
inline void Maintain(Burst& b,const centurionreload::Frame& f,centurionreload::Event event){
    if(!f.enabled||!f.context||!f.tracked||!f.supported||!f.ammoFresh||
       !f.pawn||!f.weapon||event==centurionreload::Event::Cancelled||
       (b.count&&(b.pawn!=f.pawn||b.weapon!=f.weapon||f.nowMs<b.born||f.nowMs-b.born>=650)))b={};
}
// Event-only launch: repeated rendering or held buttons cannot spawn more cases.
inline bool Launch(Burst& out,const centurionreload::Frame& f,centurionreload::Event event,
                   bool enabled,const WorldFrame& world,bool rearward=false){
    if(!enabled||!f.enabled||!f.context||!f.tracked||!f.supported||!f.ammoFresh||
       !f.pawn||!f.weapon||!centurionreload::AmmoValid(f)||f.capacity!=6||!Valid(world)||
       event!=centurionreload::Event::Dumped||!hipknife::Finite(f.socket)||!hipknife::Valid(f.gun.q))return false;
    const unsigned spent=unsigned(f.capacity-f.clip);
    if(!spent)return false;
    if(out.count&&out.pawn==f.pawn&&out.weapon==f.weapon&&out.born==f.nowMs)return false;
    Burst b;b.pawn=f.pawn;b.weapon=f.weapon;b.born=f.nowMs;b.count=spent;
    b.yaw=world.baseYaw+world.centreYaw;b.units=world.units;
    // Caller supplies the fresh rendered joint in the tracked-cylinder route;
    // legacy mode uses its authored gesture socket. Original mesh/material are
    // reused, but the native particle simulator is NOT cloned.
    for(unsigned i=0;i<b.count;++i){
        const float a=float(i)*1.0471975512f;
        const V radial{std::cos(a)*.005f,std::sin(a)*.005f,0};
        const V velocity=hipknife::Rotate(f.gun.q,{radial.x*5.f,radial.y*5.f,rearward?.35f:.22f})+V{0,rearward?-.05f:-.15f,0};
        b.cases[i]={ToWorld(f.socket+hipknife::Rotate(f.gun.q,radial),world),WorldDelta(velocity,world),
                    hipknife::Multiply(f.gun.q,centurionclip::Degrees(17.f*i,43.f*i,31.f*i))};
    }
    out=b;return true;
}
inline unsigned Sample(const Burst& b,const WorldFrame& world,std::uint32_t pawn,std::uint32_t weapon,
                       std::uint64_t now,Pose (&out)[6]){
    if(!b.count||b.count>6||!pawn||!weapon||b.pawn!=pawn||b.weapon!=weapon||now<b.born||
       now-b.born>=650||!Valid(world)||world.units!=b.units)return 0;
    const float t=float(now-b.born)*.001f,delta=world.baseYaw+world.centreYaw-b.yaw;
    const Q yaw{0,std::sin(delta*.5f),0,std::cos(delta*.5f)};
    for(unsigned i=0;i<b.count;++i){
        const auto& p=b.cases[i];
        if(!hipknife::Finite(p.origin)||!hipknife::Finite(p.velocity)||!hipknife::Valid(p.orientation))return 0;
        const V worldPoint=p.origin+p.velocity*t+V{0,0,-4.905f*b.units*t*t};
        const Q spin=centurionclip::Degrees((150.f+17.f*i)*t,(210.f-13.f*i)*t,93.f*t);
        out[i]={ToTracking(worldPoint,world),hipknife::Multiply(yaw,hipknife::Multiply(p.orientation,spin))};
        if(!hipknife::Finite(out[i].p)||!hipknife::Valid(out[i].q))return 0;
    }
    return b.count;
}
} // namespace centurionshells
