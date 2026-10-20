#pragma once
#include "weapon_muzzle_geometry.h"
namespace riflefx {
// Native PlayMuzzleFX takes (const FVector&, UBOOL), not (bool, vector).
// Only the local player's looping Valkyrie emitter is in scope. AI, pistol,
// tracer endpoints, impact effects and template pointers remain unchanged.
inline bool Origin(int mode,bool scene,bool firstPerson,std::uint32_t owner,
                   std::uint32_t pawn,std::uint32_t weapon,std::uint64_t now,
                   const weaponmuzzle::Witness& witness,hipknife::V camera,
                   hipknife::V original,hipknife::V& result) {
    if(mode!=2||!scene||!firstPerson||!owner||owner!=weapon||
       witness.kind!=weaponreload::Kind::Valkyrie||!witness.Current(pawn,weapon,now)||
       !hipknife::Finite(camera)||!hipknife::Finite(original))return false;
    const auto position=camera+witness.relative;
    // Refuse a different world/origin epoch; this is not an offset calibration.
    if(!hipknife::Finite(position)||hipknife::Length(position-original)>520.f)return false;
    result=position;return true;
}
}
