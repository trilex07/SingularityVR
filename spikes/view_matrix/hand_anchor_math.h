#pragma once
#include "arm_bind.h"
namespace handanchor {
using MatrixRows = const float (*)[4];
inline bool PistolGrip(bool gloves,bool liveWeapon,bool holstered,long gunVertices,bool supportGrip) {
    return gloves && liveWeapon && !holstered && gunVertices==5799 && !supportGrip;
}
inline bool ValkyrieSingleGrip(bool enabled,bool gloves,bool liveWeapon,bool holstered,
                               long gunVertices,bool supportGrip) {
    return enabled && gloves && liveWeapon && !holstered && gunVertices==11387 && !supportGrip;
}
inline bool VolkSingleGrip(bool enabled,bool gloves,bool liveWeapon,bool holstered,
                           long gunVertices,bool supportGrip) {
    return enabled && gloves && liveWeapon && !holstered && gunVertices==8297 && !supportGrip;
}
inline bool KasimovSingleGrip(bool enabled,bool gloves,bool liveWeapon,bool holstered,
                              long gunVertices,bool supportGrip) {
    return enabled && gloves && liveWeapon && !holstered && gunVertices==11389 && !supportGrip;
}
// Material-0 TMD gloves contain the same right hand as the no-TMD foreground
// section. While a right-hand weapon is held, both must consume its final
// BuildGunC target; otherwise the TMD glove follows the bare controller while
// the visible weapon follows its authored grip/align transform.
inline bool WeaponGripSection(bool noTmdGloves,bool tmdGloves,bool dualContext) {
    return noTmdGloves || (tmdGloves && dualContext);
}
// Free hands miss the pads by the measured gun-anchor residual (~50 UU left,
// ~41 UU right). That correction runs only while no weapon is live. A held
// pistol keeps BuildGunC through AimTarget.
inline bool FreeWristAnchor(bool gloves, bool weaponLive, int isolateMode, bool supportGrip) {
    return gloves && !weaponLive && isolateMode == 2 && !supportGrip;
}
// TMD material 0 also contains the original right wrist (bone 51). During
// Impulse the native right pose can park near the body while its controller
// stays tracked; the noTMD free-hand correction does not cover this section.
inline bool TmdRightWristAnchor(bool enabled,bool tmdGloves,bool dual,bool tmdHands,
                                bool rightAim,bool rightTracked,bool supportGrip,int isolateMode) {
    return enabled&&tmdGloves&&dual&&tmdHands&&rightAim&&rightTracked&&
        !supportGrip&&isolateMode==2;
}
// Select at consumption time: a snapshot made before TargetWrist silently
// bypasses its correction. The proven pistol grip deliberately keeps its
// weapon target; free hands use the final corrected controller target.
inline MatrixRows AimTarget(bool pistolGrip, const float weapon[4][4],
                            const float corrected[4][4]) {
    return pistolGrip ? weapon : corrected;
}
inline armik::V Point(armik::V p,const float m[4][4]) {
    return {p.x*m[0][0]+p.y*m[1][0]+p.z*m[2][0]+m[3][0],
            p.x*m[0][1]+p.y*m[1][1]+p.z*m[2][1]+m[3][1],
            p.x*m[0][2]+p.y*m[1][2]+p.z*m[2][2]+m[3][2]};
}
inline bool Affine(const float m[4][4]) {
    for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)if(!std::isfinite(m[r][c]))return false;
    return std::fabs(m[0][3])+std::fabs(m[1][3])+std::fabs(m[2][3])+std::fabs(m[3][3]-1.f)<1e-5f;
}
// Skin constants are inverse-bind * current-joint, NOT the joint position itself.
// Preserve controller rotation/scale; only remove the measured wrong pivot offset.
inline bool TargetWrist(float out[4][4],const armbind::Mat& bind,const float skin[3][4],
                        const float local[4][4],const float controller[4][4],armik::V desired) {
    if(!Affine(local)||!Affine(controller)||!armik::Finite(desired))return false;
    armbind::Mat joint;if(!armbind::CurrentJoint(bind,skin,joint))return false;
    const auto nativeWorld=Point(armbind::Position(joint),local);
    const auto oldTarget=Point(nativeWorld,controller);
    if(!armik::Finite(nativeWorld)||!armik::Finite(oldTarget))return false;
    float staged[4][4];std::memcpy(staged,controller,sizeof(staged));
    staged[3][0]+=desired.x-oldTarget.x;staged[3][1]+=desired.y-oldTarget.y;staged[3][2]+=desired.z-oldTarget.z;
    if(!Affine(staged))return false;
    std::memcpy(out,staged,sizeof(staged));return true;
}
inline bool GameplaySource(bool enabled,bool cinematic,bool menu,bool paused,bool gameplay) {
    return enabled && !cinematic && !menu && !paused && gameplay;
}
}
