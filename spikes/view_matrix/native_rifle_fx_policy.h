#pragma once
#include "weapon_muzzle_geometry.h"
namespace riflefx {
struct Attachment {std::uint32_t base=0,mesh=0,name=0,number=0;};
inline Attachment WorldMuzzleAttachment(bool measuredLocalSpawn,Attachment original){
    // ObjectPool::GetEmitterEx attaches AFTER setting the requested world pose.
    // Actor::SetBase replaces relative pose with the named socket's transform.
    // A controller-owned world muzzle must not inherit that native attachment.
    return measuredLocalSpawn?Attachment{}:original;
}
inline bool SpawnDirection(bool enabled,bool scoped,bool exactCaller,
    std::uint32_t pawn,std::uint32_t weapon,std::uint64_t now,
    const weaponmuzzle::Witness& witness,hipknife::V& result){
    return enabled&&scoped&&exactCaller&&witness.Direction(pawn,weapon,now,result);
}
// Native PlayMuzzleFX takes (const FVector&, UBOOL), not (bool, vector).
// Valkyrie follow-through is unchanged. Optional measured pistol/sniper spawn
// correction uses this same native FVector argument. AI,
// tracer endpoints, impact effects and template pointers remain unchanged.
inline bool Origin(int mode,bool scene,bool firstPerson,std::uint32_t owner,
                   std::uint32_t pawn,std::uint32_t weapon,std::uint64_t now,
                   const weaponmuzzle::Witness& witness,hipknife::V camera,
                   hipknife::V original,hipknife::V& result,
                   bool measuredOther=false,unsigned meshVertices=0) {
    const bool eligible=witness.kind==weaponreload::Kind::Valkyrie ||
        (measuredOther&&((meshVertices==11389&&witness.kind==weaponreload::Kind::Kasimov)||
                         (meshVertices==5799&&witness.kind==weaponreload::Kind::None)));
    if((mode!=2&&mode!=3)||!scene||!firstPerson||!owner||owner!=weapon||
       !eligible||!witness.Current(pawn,weapon,now)||
       !hipknife::Finite(camera)||!hipknife::Finite(original))return false;
    const auto position=camera+witness.relative;
    // Refuse a different world/origin epoch; this is not an offset calibration.
    if(!hipknife::Finite(position)||hipknife::Length(position-original)>520.f)return false;
    result=position;return true;
}
// A firing call leases an existing native emitter briefly; movement must never
// extend that lease or restart a particle system after the game stops firing.
inline bool Lease(int mode,bool scene,std::uint64_t fired,std::uint64_t now,
                  std::uint32_t pawn,std::uint32_t weapon,const weaponmuzzle::Witness& w){
    return mode==3&&scene&&fired&&now>=fired&&now-fired<=150&&
        w.kind==weaponreload::Kind::Valkyrie&&w.Current(pawn,weapon,now);
}
inline bool Rotation(hipknife::V direction,int& pitch,int& yaw){
    const auto length=hipknife::Length(direction);
    if(!hipknife::Finite(direction)||!std::isfinite(length)||length<.99f||length>1.01f)return false;
    constexpr float uu=65536.f/6.28318530718f;
    pitch=(int)std::lround(std::atan2(direction.z,std::sqrt(direction.x*direction.x+direction.y*direction.y))*uu);
    yaw=(int)std::lround(std::atan2(direction.y,direction.x)*uu);
    return true;
}
inline bool TracerSource(bool enabled,bool scene,bool callsite,std::uintptr_t object,
                         std::uint32_t pawn,std::uint32_t weapon,std::uint64_t now,
                         const weaponmuzzle::Witness& w,hipknife::V camera,
                         hipknife::V original,hipknife::V& result){
    // +0x2d4 is the source-interface subobject proven by the owned constructor
    // and both branches of the complete getter, not a guessed actor field.
    return enabled&&callsite&&weapon&&object==std::uintptr_t(weapon)+0x2d4&&
        Origin(3,scene,true,weapon,pawn,weapon,now,w,camera,original,result);
}
}
