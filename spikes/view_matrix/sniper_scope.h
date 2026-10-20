#pragma once
#include "hip_knife.h"

// Pure tracking-space policy. The mount comes from an audited native mesh draw,
// not a guessed controller-to-eye offset. Keep the mount while the native scope
// hides the mesh; otherwise a draw-age timeout would repeatedly drop zoom.
namespace sniperscope {
using hipknife::V;using hipknife::Pose;
struct Mount {
    bool valid=false;std::uint32_t pawn=0,weapon=0;
    V lens{},forward{}; // physical aim-controller LOCAL metres
};
inline bool Capture(Mount& out,std::uint32_t pawn,std::uint32_t weapon,
                    Pose hand,V lens,V ahead) {
    if(!pawn||!weapon||!hipknife::Valid(hand.q)||!hipknife::Finite(hand.p)||
       !hipknife::Finite(lens)||!hipknife::Finite(ahead))return false;
    V f=ahead-lens;const float length=hipknife::Length(f);
    if(length<.001f||length>2.f||hipknife::Length(lens-hand.p)>.9f)return false;
    const hipknife::Q inverse{-hand.q.x,-hand.q.y,-hand.q.z,hand.q.w};
    Mount m{true,pawn,weapon,hipknife::Rotate(inverse,lens-hand.p),
        hipknife::Rotate(inverse,f*(1.f/length))};
    out=m;return true;
}
struct State {
    bool active=false;std::uint64_t entered=0;
    std::uint32_t ownerPawn=0,ownerWeapon=0;
    Mount trackingMount{};
    float axial=0,radial=0,alignment=0;int eyeIndex=-1;
    bool Update(bool eligible,const Mount& m,std::uint32_t pawn,std::uint32_t weapon,
                Pose hand,const Pose (&eyes)[2],std::uint64_t now) {
        if(pawn!=ownerPawn||weapon!=ownerWeapon){active=false;entered=0;trackingMount={};ownerPawn=pawn;ownerWeapon=weapon;}
        // Native aim may change the render-side bone pose before its overlay
        // is visible. Do not feed that animation back into the proximity sensor:
        // retain the controller-local mount for this dwell/hold transaction.
        if(!entered&&!active)trackingMount=m;
        axial=radial=alignment=0;eyeIndex=-1;
        bool close=false;
        if(eligible&&trackingMount.valid&&pawn&&weapon&&trackingMount.pawn==pawn&&trackingMount.weapon==weapon&&
           hipknife::Valid(hand.q)&&hipknife::Finite(hand.p)) {
            const V lens=hand.p+hipknife::Rotate(hand.q,trackingMount.lens);
            const V f=hipknife::Rotate(hand.q,trackingMount.forward);
            for(unsigned e=0;e<2;++e)if(hipknife::Valid(eyes[e].q)&&hipknife::Finite(eyes[e].p)) {
                const auto& eye=eyes[e];
                const V d=lens-eye.p;const float relief=hipknife::Dot(d,f);
                const float radial2=hipknife::Dot(d,d)-relief*relief;
                const float radiusNow=std::sqrt(radial2>0?radial2:0.f);
                const float dot=hipknife::Dot(f,hipknife::Rotate(eye.q,{0,0,-1}));
                if(eyeIndex<0||radiusNow<radial){eyeIndex=int(e);axial=relief;radial=radiusNow;alignment=dot;}
                // Eye relief and alignment, with separate exit bounds. These
                // are interaction tolerances, not changes to native zoom/FOV.
                const float radius=active?.07f:.045f;
                close|=relief>.015f&&relief<(active?.32f:.24f)&&radial2<radius*radius&&dot>(active?.90f:.96f);
            }
        }
        if(!close){active=false;entered=0;return false;}
        if(active)return true;
        if(!entered||now<entered)entered=now;
        active=now>=entered&&now-entered>=100;
        return active;
    }
};
}
