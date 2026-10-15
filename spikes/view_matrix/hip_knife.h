#pragma once
// Opt-in pre-TMD knife gesture. Native damage/trace stays owned by UE3.
#include "ui_pointer.h"
#include <cstdint>

namespace hipknife {
using V=uipointer::V;
using uipointer::Dot;
using uipointer::Finite;
struct Q {float x=0,y=0,z=0,w=1;};
struct Pose {V p;Q q;};
inline Q Multiply(Q a,Q b){return {
    a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,
    a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,
    a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w,
    a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z};}
inline bool Valid(Q q){
    const float n=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
    return std::isfinite(n)&&std::fabs(n-1.f)<.02f;
}
inline V Rotate(Q q,V v){
    const V a{q.x,q.y,q.z};
    const V t{2*(a.y*v.z-a.z*v.y),2*(a.z*v.x-a.x*v.z),2*(a.x*v.y-a.y*v.x)};
    return v+t*q.w+V{a.y*t.z-a.z*t.y,a.z*t.x-a.x*t.z,a.x*t.y-a.y*t.x};
}
inline V EyeLocal(V world,const Pose& eye){return Rotate({-eye.q.x,-eye.q.y,-eye.q.z,eye.q.w},world-eye.p);}
inline float Length(V v){return std::sqrt(Dot(v,v));}
struct Config {
    // Tracking metres: left is literal physical left, never mirrored by AimHand.
    float left=.25f,down=.50f,forward=.06f,radius=.15f;
    float minSpeed=1.2f,resetSpeed=.35f,minTravel=.10f;
    std::uint64_t pickupGraceMs=300,cooldownMs=750,pulseMs=90;
};
// Only yaw, not head pitch/roll; looking down must not throw the belt forward.
inline Pose Hip(V head,float bodyYaw,const Config& c={}){
    const Q q{0,std::sin(bodyYaw*.5f),0,std::cos(bodyYaw*.5f)};
    return {head+Rotate(q,{-c.left,-c.down,-c.forward}),q};
}
enum class Event {None,Picked,Sheathed,Reset,Swing};
struct Frame {
    bool enabled=false,gameplay=false,focused=false,alive=false,nativeKnife=false;
    // MAIN round 14: post-TMD mode. TMD!=0 with intact impulse mapping.
    // Pickup then restricted to physical right hand (h==1) with no weapon
    // in hand; pre-TMD path (nativeKnife) unchanged.
    bool hasTMD=false,noWeapon=false;
    bool tracked[2]{},grip[2]{},free[2]{};
    Pose hand[2];V head;Pose hip;
    std::uint64_t nowMs=0;std::uint32_t pawn=0;
};
struct State {
    int held=-1;bool priorGrip[2]{true,true};bool armed=false,sampled=false;
    V previousTip{},strokeOrigin{};
    std::uint64_t sampleMs=0,pickedMs=0,lastSwingMs=0,pulseUntil=0;
    std::uint32_t pawn=0;
    bool Pulse(std::uint64_t now)const{return held>=0&&now<pulseUntil;}
};
inline bool Context(const Frame& f){return f.enabled&&f.gameplay&&f.focused&&f.alive&&
    (f.nativeKnife||f.hasTMD)&&f.pawn&&Finite(f.head)&&Finite(f.hip.p)&&Valid(f.hip.q);}
inline Event Update(State& s,const Frame& f,const Config& c={}){
    const bool valid=Context(f)&&(!s.pawn||s.pawn==f.pawn);
    if(!valid){const bool had=s.held>=0;s={};s.pawn=f.pawn;return had?Event::Reset:Event::None;}
    // Post-TMD (hasTMD without nativeKnife): a left-held knife from before
    // the TMD arrived must not survive; right-hand-only from here.
    if(f.hasTMD&&!f.nativeKnife&&s.held>=0&&s.held!=1){s={};s.pawn=f.pawn;return Event::Reset;}
    s.pawn=f.pawn;
    bool edge[2]{};
    for(int h=0;h<2;++h){edge[h]=f.grip[h]&&!s.priorGrip[h];s.priorGrip[h]=f.grip[h];}
    if(s.held<0){
        const bool postTMD=f.hasTMD&&!f.nativeKnife;
        for(int h=0;h<2;++h)if(edge[h]&&f.free[h]&&f.tracked[h]&&Finite(f.hand[h].p)&&Valid(f.hand[h].q)&&
            Length(f.hand[h].p-f.hip.p)<=c.radius&&(!postTMD||(h==1&&f.noWeapon))){
            s.held=h;s.pickedMs=f.nowMs;s.sampled=false;s.armed=false;s.pulseUntil=0;return Event::Picked;
        }
        return Event::None;
    }
    const int h=s.held;
    if(!f.tracked[h]||!f.free[h]||!Finite(f.hand[h].p)||!Valid(f.hand[h].q)){
        s.held=-1;s.sampled=false;s.armed=false;s.pulseUntil=0;return Event::Reset;
    }
    if(!f.grip[h]){s.held=-1;s.sampled=false;s.armed=false;s.pulseUntil=0;return Event::Sheathed;}
    // Tip includes angular swings. No engine/view writes; tracking coordinates
    // do not change from stick locomotion or software snap-turns.
    const V tip=f.hand[h].p+Rotate(f.hand[h].q,{0,0,-.22f});
    if(!s.sampled||f.nowMs<=s.sampleMs||f.nowMs-s.sampleMs>80){
        s.sampled=true;s.sampleMs=f.nowMs;s.previousTip=s.strokeOrigin=tip;s.armed=false;s.pulseUntil=0;return Event::None;
    }
    const float dt=float(f.nowMs-s.sampleMs)*.001f;
    const float delta=Length(tip-s.previousTip);
    s.previousTip=tip;s.sampleMs=f.nowMs;
    if(delta>.25f){s.armed=false;s.strokeOrigin=tip;s.pulseUntil=0;return Event::None;} // tracking jump
    const float speed=delta/dt;
    if(speed<c.resetSpeed){s.strokeOrigin=tip;s.armed=true;}
    const bool away=Length(f.hand[h].p-f.hip.p)>c.radius+.06f;
    if(s.armed&&away&&f.nowMs-s.pickedMs>=c.pickupGraceMs&&
       (!s.lastSwingMs||f.nowMs-s.lastSwingMs>=c.cooldownMs)&&
       speed>=c.minSpeed&&Length(tip-s.strokeOrigin)>=c.minTravel){
        s.armed=false;s.lastSwingMs=f.nowMs;s.pulseUntil=f.nowMs+c.pulseMs;return Event::Swing;
    }
    return Event::None;
}
// Leave ALL normal inputs untouched while docked. Holding a knife reserves its
// hand: no accidental gun fire/reload/TMD/support grip from the same squeeze.
inline bool Reserves(const State& s,int hand){return s.held==hand;}
struct TriggerGate {
    bool blocked=false;
    float Update(bool reserved,float value){
        if(reserved)blocked=true;
        else if(value<=.1f)blocked=false;
        return blocked?0.f:value;
    }
};
inline bool Projection(float tx,float ty,float (&m)[4][4]){
    if(!std::isfinite(tx)||!std::isfinite(ty)||tx<=0||ty<=0)return false;
    for(auto& row:m)for(float& v:row)v=0;
    const float nearZ=.015f,farZ=10.f;
    m[0][0]=1/tx;m[1][1]=1/ty;m[2][2]=farZ/(nearZ-farZ);m[2][3]=-1;
    m[3][2]=nearZ*farZ/(nearZ-farZ);return true;
}
} // namespace hipknife
