#pragma once
// CPU-only transaction policy. Does not spawn/grant inventory, call UE3,
// consume native input, or start a grenade timer. Backends must acknowledge
// identity-bound commands. Until then this is NOT a playable grenade feature.
#include "hip_knife.h"
namespace bodyequipment {
using hipknife::V;
using hipknife::Pose;
enum class Slot { None,Ammo,Grenade,Heavy,Knife,Gun };
struct Zone {Slot slot=Slot::None;V centre{};float radius=0;};
struct Layout {
    // OpenXR metres: +X right, +Y up, +Z behind. Yaw only. These NEW
    // anchors are provisional; never overwrite the accepted ammo/hip config.
    V grenade{.22f,-.12f,.18f},heavy{.18f,-.55f,.32f};
    float grenadeRadius=.18f,heavyRadius=.16f;
    Zone At(Slot slot,V head,float yaw)const{
        const hipknife::Q q{0,std::sin(yaw*.5f),0,std::cos(yaw*.5f)};
        const bool g=slot==Slot::Grenade;
        if((!g&&slot!=Slot::Heavy)||!hipknife::Finite(head)||!std::isfinite(yaw))return {};
        return {slot,head+hipknife::Rotate(q,g?grenade:heavy),g?grenadeRadius:heavyRadius};
    }
};
// Resolve a rising grip to ONE nearest slot; ambiguous overlap refuses. The
// caller supplies accepted ammo/knife/gun zones, not hardcoded replacements.
inline Slot Pick(V hand,const Zone* zones,unsigned count){
    if(!hipknife::Finite(hand)||!zones||count>8)return Slot::None;
    Slot best=Slot::None;float distance=1000,second=1000;
    for(unsigned i=0;i<count;++i){
        const auto& z=zones[i];
        if(z.slot==Slot::None||!hipknife::Finite(z.centre)||!std::isfinite(z.radius)||z.radius<=0||z.radius>.5f)continue;
        const float d=hipknife::Length(hand-z.centre);
        if(d>z.radius)continue;
        if(d<distance){second=distance;distance=d;best=z.slot;}
        else if(d<second)second=d;
    }
    return second-distance<=.025f?Slot::None:best;
}
enum class Command {None,DrawGrenade,PullPin,ThrowGrenade,ReturnGrenade,DrawHeavy,StowHeavy};
struct Request {
    Command command=Command::None;std::uint32_t sequence=0,pawn=0,item=0;
    V position{},velocity{};
};
struct Ack {std::uint32_t sequence=0,pawn=0,item=0;Command command=Command::None;bool success=false;};
struct Frame {
    bool enabled=false,gameplay=false,focused=false,alive=false;
    bool tracked[2]{},grip[2]{},free[2]{};Pose hand[2]{};
    // Native owner supplies genuine owned item identities. Zero = unavailable.
    std::uint32_t pawn=0,grenade=0,heavy=0;std::uint64_t now=0;
    Slot rightSlot=Slot::None;
    V pin{};bool pinVerified=false; // actual native/asset pin in tracking metres
    Ack ack{};
};
struct Config {
    float pinRadius=.07f,pullDistance=.09f,minThrowSpeed=.65f,maxThrowSpeed=12.f;
    std::uint64_t pendingMs=1000;
};
enum class Phase {Idle,DrawGrenade,Safe,PinPull,ArmPending,Armed,ThrowPending,ReturnPending,DrawHeavy,Heavy,StowHeavy};
struct State {
    Phase phase=Phase::Idle;std::uint32_t pawn=0,item=0,serial=0;
    bool priorGrip[2]{true,true},sampled=false;
    V previous{},velocity{},pinStart{};
    std::uint64_t sampleMs=0,since=0;
    Request pending{};
    Request Issue(Command cmd,const Frame& f){
        if(++serial==0)++serial;
        pending={cmd,serial,f.pawn,item,f.hand[1].p,velocity};since=f.now;return pending;
    }
    void Clear(){const auto generation=serial;*this={};serial=generation;}
};
inline bool Context(const Frame& f){
    return f.enabled&&f.gameplay&&f.focused&&f.alive&&f.pawn&&f.tracked[1]&&
        hipknife::Finite(f.hand[1].p)&&hipknife::Valid(f.hand[1].q);
}
inline bool Matches(const Request& r,const Ack& a){
    return r.command!=Command::None&&r.command==a.command&&r.sequence&&
        r.sequence==a.sequence&&r.pawn==a.pawn&&r.item&&r.item==a.item;
}
inline bool Pending(Phase p){return p==Phase::DrawGrenade||p==Phase::ArmPending||
    p==Phase::ThrowPending||p==Phase::ReturnPending||p==Phase::DrawHeavy||p==Phase::StowHeavy;}
inline Request Update(State& s,const Frame& f,const Config& c={}){
    // Losing focus/tracking never synthesizes a throw or extinguishes a fuse.
    // Backend owns any live armed object and its native lifetime, not this FSM.
    if(!Context(f)||(s.pawn&&s.pawn!=f.pawn)) {s.Clear();return {};}
    s.pawn=f.pawn;
    bool edge[2]{};
    for(unsigned h=0;h<2;++h){edge[h]=f.grip[h]&&!s.priorGrip[h];s.priorGrip[h]=f.grip[h];}
    const bool grenadePhase=s.phase!=Phase::Idle&&s.phase!=Phase::DrawHeavy&&s.phase!=Phase::Heavy&&s.phase!=Phase::StowHeavy;
    if(s.phase!=Phase::Idle&&s.item!=(grenadePhase?f.grenade:f.heavy)){s.Clear();return {};}
    if(Pending(s.phase)){
        if(Matches(s.pending,f.ack)){
            const auto cmd=s.pending.command;s.pending={};
            if(!f.ack.success){s.Clear();return {};}
            if(cmd==Command::DrawGrenade){s.phase=Phase::Safe;s.sampled=false;}
            else if(cmd==Command::PullPin){s.phase=Phase::Armed;s.sampled=false;}
            else if(cmd==Command::DrawHeavy)s.phase=Phase::Heavy;
            else {s.Clear();return {};}
        }else if(f.now<s.since||f.now-s.since>c.pendingMs){s.Clear();return {};}
        else return {}; // no duplicate native calls on held input
    }
    if(s.phase==Phase::Idle){
        if(!edge[1]||!f.free[1])return {};
        if(f.rightSlot==Slot::Grenade&&f.grenade){s.item=f.grenade;s.phase=Phase::DrawGrenade;return s.Issue(Command::DrawGrenade,f);}
        if(f.rightSlot==Slot::Heavy&&f.heavy){s.item=f.heavy;s.phase=Phase::DrawHeavy;return s.Issue(Command::DrawHeavy,f);}
        return {};
    }
    if(s.phase==Phase::Heavy){
        if(edge[1]&&f.rightSlot==Slot::Heavy){s.phase=Phase::StowHeavy;return s.Issue(Command::StowHeavy,f);}
        return {}; // support/R2/native reload belong to the native weapon
    }
    if(!f.grip[1]){
        if(s.phase==Phase::Armed&&s.sampled&&f.now>=s.sampleMs&&f.now-s.sampleMs<=50&&
           hipknife::Length(s.velocity)>=c.minThrowSpeed){
            s.phase=Phase::ThrowPending;return s.Issue(Command::ThrowGrenade,f);
        }
        if(s.phase!=Phase::Armed){s.phase=Phase::ReturnPending;return s.Issue(Command::ReturnGrenade,f);}
        s.Clear();return {}; // never invent a safe return for an armed object
    }
    if(s.phase==Phase::Safe&&edge[0]&&f.free[0]&&f.tracked[0]&&f.pinVerified&&
       hipknife::Finite(f.pin)&&hipknife::Finite(f.hand[0].p)&&
       hipknife::Length(f.hand[0].p-f.pin)<=c.pinRadius){
        s.phase=Phase::PinPull;
        const hipknife::Q inv{-f.hand[1].q.x,-f.hand[1].q.y,-f.hand[1].q.z,f.hand[1].q.w};
        s.pinStart=hipknife::Rotate(inv,f.hand[0].p-f.hand[1].p);
    }
    if(s.phase==Phase::PinPull){
        if(!f.grip[0]||!f.tracked[0]||!f.free[0]||!f.pinVerified||!hipknife::Finite(f.hand[0].p))s.phase=Phase::Safe;
        else {
            const auto& q=f.hand[1].q;const hipknife::Q inv{-q.x,-q.y,-q.z,q.w};
            const auto relative=hipknife::Rotate(inv,f.hand[0].p-f.hand[1].p);
            // Moving/rotating both hands together must NOT pull the pin.
            if(hipknife::Length(relative-s.pinStart)>=c.pullDistance){s.phase=Phase::ArmPending;return s.Issue(Command::PullPin,f);}
        }
    }
    if(s.phase==Phase::Armed){
        if(s.sampled&&f.now>s.sampleMs&&f.now-s.sampleMs<=50){
            const float dt=float(f.now-s.sampleMs)*.001f;
            const V v=(f.hand[1].p-s.previous)*(1.f/dt);
            const float speed=hipknife::Length(v);
            if(std::isfinite(speed)&&speed<=c.maxThrowSpeed)s.velocity=v*.5f+s.velocity*.5f;
            else s.velocity={};
        }else s.velocity={};
        s.sampled=true;s.sampleMs=f.now;s.previous=f.hand[1].p;
    }
    return {};
}
}
