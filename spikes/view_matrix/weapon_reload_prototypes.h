#pragma once
#include "centurion_reload.h"
#include "weapon_reload_profiles.h"
#include <algorithm>
#include <cstring>
#include <limits>

// Pure CPU policy, called by the opt-in runtime dispatcher. No engine ABI here.
// Independent magazine policy: Circle eject -> shoulder grab -> insert/release.
// Reuse observation types, NEVER the Centurion cylinder FSM or native tap.
namespace weaponreloadproto {
// A disabled post-TMD knife has no grip or proximity claim on Valkyrie's
// shoulder pouch. Keep this policy profile-specific: Volk/Kasimov retain their
// previously tested routing until a separate physical result warrants change.
inline bool ValkyrieKnifeCanBlock(bool enabled,bool ready,bool disabledByTmd){
    return enabled&&ready&&!disabledByTmd;
}
struct Shoulder {float left=.22f,down=.12f,back=.18f,leftExtension=0;};
inline bool MidEye(hipknife::V left,hipknife::V right,hipknife::V& centre){
    if(!hipknife::Finite(left)||!hipknife::Finite(right))return false;
    const float separation=hipknife::Length(right-left);
    if(separation<.005f||separation>.20f)return false;
    centre=(left+right)*.5f;return true;
}
inline hipknife::Pose Supply(hipknife::V headCentre,float latchedPhysicalYaw,const Shoulder& c){
    const hipknife::Q q{0,std::sin(latchedPhysicalYaw*.5f),0,std::cos(latchedPhysicalYaw*.5f)};
    return {headCentre+hipknife::Rotate(q,{-c.left,-c.down,c.back}),q};
}
// Original spherical pouch plus a capsule going only toward physical left.
// The segment begins at the OLD centre, so no previously valid point is lost.
inline float ShoulderDistance(hipknife::V hand,hipknife::V originalCentre,
                              float physicalYaw,const Shoulder& c){
    if(!hipknife::Finite(hand)||!hipknife::Finite(originalCentre)||
       !std::isfinite(physicalYaw)||!std::isfinite(c.leftExtension))
        return std::numeric_limits<float>::infinity();
    if(c.leftExtension<=0)return hipknife::Length(hand-originalCentre);
    const hipknife::Q q{0,std::sin(physicalYaw*.5f),0,std::cos(physicalYaw*.5f)};
    const auto span=hipknife::Rotate(q,{-c.leftExtension,0,0});
    const float length2=hipknife::Dot(span,span);
    if(!std::isfinite(length2)||length2<=0)return std::numeric_limits<float>::infinity();
    const float along=std::clamp(hipknife::Dot(hand-originalCentre,span)/length2,0.f,1.f);
    return hipknife::Length(hand-(originalCentre+span*along));
}
struct Calibration {
    bool ready=false;
    std::uint32_t revision=0; // caller increments on any calibration change
    centurionreload::Config gesture{}; // INERT until explicitly calibrated
    Shoulder shoulder{}; // physical left/back; caller must use Supply, not old belt Pouch
    std::uint64_t gripReachMs=800; // bounded squeeze-before-reach, profile-owned
};
inline bool Valid(const Calibration& c){
    const auto& g=c.gesture;
    return c.ready&&c.revision&&hipknife::Finite(g.socketLocal)&&
        hipknife::Length(g.socketLocal)<=1.f&&std::isfinite(c.shoulder.left)&&
        std::isfinite(c.shoulder.down)&&std::isfinite(c.shoulder.back)&&
        std::isfinite(c.shoulder.leftExtension)&&
        c.shoulder.left>=.15f&&c.shoulder.left<=.40f&&c.shoulder.down>=0&&c.shoulder.down<=.25f&&
        c.shoulder.back>=.10f&&c.shoulder.back<=.35f&&
        c.shoulder.leftExtension>=0&&c.shoulder.leftExtension<=.25f&&c.gripReachMs<=3000&&
        g.socketRadius>0&&g.socketRadius<g.pullDistance&&g.pullDistance<=1.f&&
        g.pouchRadius>0&&g.pouchRadius<=.5f&&g.timeoutMs>=100&&g.timeoutMs<=60000&&
        g.ackMs>=100&&g.ackMs<=60000&&g.cooldownMs<=5000;
}
inline bool Same(const centurionreload::Config& a,const centurionreload::Config& b){
    return a.pouchRadius==b.pouchRadius&&
        a.socketLocal.x==b.socketLocal.x&&a.socketLocal.y==b.socketLocal.y&&a.socketLocal.z==b.socketLocal.z&&
        a.socketRadius==b.socketRadius&&a.pullDistance==b.pullDistance&&
        a.timeoutMs==b.timeoutMs&&a.ackMs==b.ackMs&&a.cooldownMs==b.cooldownMs;
}
struct Frame {
    bool enabled=false; // independent per-profile caller opt-in, default OFF
    bool liveIdentityVerified=false; // cannot be inferred from wedge count
    Kind kind=Kind::None;
    const char* objectName=nullptr; // resolved base FName, not a class substring
    long meshWedges=0;
    int aimHand=1; // first slice: right weapon, physical left interaction only
    float bodyYaw=0; // physical head yaw for the independent shoulder anchor
    bool magazineReleaseDown=false; // named action LEVEL, PSVR2 binding: Circle
    bool nativePermitRequired=false,nativePermitConsumed=false; // real dispatcher evidence, not just conserved HUD change
    bool measuredInsertion=false,insertionValid=false;
    hipknife::V insertionPoint{}; // carried clip bind origin, NOT controller origin
    bool seekerPath=false,seekerApproach=false,seekerNear=false; // source-axis and orientation gates
    centurionreload::Frame gesture{};
};
enum class Phase {Idle,NeedAmmo,Carrying,WaitingNative,Cooldown};
enum class Event {None,Ejected,GrabFresh,Discarded,Inserted,Acknowledged,Rejected,Cancelled};
inline const char* Name(Event e){switch(e){
    case Event::Ejected:return "magazine-ejected";case Event::GrabFresh:return "grab-fresh";
    case Event::Discarded:return "discarded";case Event::Inserted:return "inserted/request";
    case Event::Acknowledged:return "native-ammo-ack";case Event::Rejected:return "native-timeout";
    case Event::Cancelled:return "cancelled";default:return "none";
}}
struct Transaction {
    Phase phase=Phase::Idle;
    bool priorGrip=true,ownsGrip=false,releaseArmed=false;
    std::uint32_t pawn=0,weapon=0;
    int startClip=-1,startReserve=-1,startCapacity=-1;
    std::uint64_t since=0,lastMs=0;
    std::uint64_t reachUntil=0;
    float bodyYaw=0; // physical head yaw latched on eject, not stick-turn/body yaw
    bool approachSeen=false; // Seeker: entered rear corridor before seating
};
struct State {
    Kind profile=Kind::None;
    std::uint32_t calibrationRevision=0;
    bool haveCalibration=false;
    centurionreload::Config priorCalibration{};
    Shoulder priorShoulder{};
    std::uint64_t priorGripReachMs=0;
    Transaction transaction{};
};
struct Result {
    Event event=Event::None;
    bool reservePhysicalLeft=false;
    bool reloadIntent=false; // abstract intent ONLY, never a native call
    bool ejectMagazine=false; // one visual/backend intent, NOT an inventory write
};
inline Result Update(State& s,const Frame& f,const Calibration& c){
    const auto* profile=Find(f.kind);
    const bool identity=profile&&f.liveIdentityVerified&&f.objectName&&
        !std::strcmp(f.objectName,profile->objectName)&&f.meshWedges==profile->meshWedges;
    const bool calibrated=Valid(c);
    const bool changed=(s.profile!=Kind::None&&s.profile!=f.kind)||
        (s.haveCalibration&&(s.calibrationRevision!=c.revision||!Same(s.priorCalibration,c.gesture)||
            s.priorShoulder.left!=c.shoulder.left||s.priorShoulder.down!=c.shoulder.down||
            s.priorShoulder.back!=c.shoulder.back||s.priorShoulder.leftExtension!=c.shoulder.leftExtension||
            s.priorGripReachMs!=c.gripReachMs));
    const auto& in=f.gesture;auto& t=s.transaction;
    const bool gripEdge=in.grip&&!t.priorGrip,gripRelease=!in.grip&&t.priorGrip;
    const bool core=f.enabled&&identity&&f.aimHand==1&&calibrated&&!changed&&
        in.context&&in.tracked&&in.supported&&in.pawn&&in.weapon&&in.ammoFresh&&
        centurionreload::AmmoValid(in)&&hipknife::Finite(in.hand)&&hipknife::Finite(in.socket)&&
        hipknife::Finite(in.pouch)&&(!in.firing||t.phase!=Phase::Idle)&&in.nowMs>=t.lastMs&&
        (!t.pawn||t.pawn==in.pawn)&&(!t.weapon||t.weapon==in.weapon);
    s.profile=identity?f.kind:Kind::None;
    s.calibrationRevision=calibrated?c.revision:0;s.haveCalibration=calibrated;
    if(calibrated){s.priorCalibration=c.gesture;s.priorShoulder=c.shoulder;s.priorGripReachMs=c.gripReachMs;}
    if(!core){
        const bool active=t.phase!=Phase::Idle&&t.phase!=Phase::Cooldown;
        const bool owned=t.ownsGrip&&in.grip;
        t={};t.priorGrip=in.grip;t.ownsGrip=owned;t.lastMs=in.nowMs;
        return {active?Event::Cancelled:Event::None,owned,false,false};
    }
    t.pawn=in.pawn;t.weapon=in.weapon;t.lastMs=in.nowMs;t.priorGrip=in.grip;
    if(!in.grip)t.ownsGrip=false;
    // A discrete semantic action, requiring neutral in an ELIGIBLE context.
    // Never fire from a held button inherited from a menu/identity/tracking gap.
    const bool eject=f.magazineReleaseDown&&t.releaseArmed;
    if(!f.magazineReleaseDown)t.releaseArmed=true;
    if(eject)t.releaseArmed=false;
    auto out=[&](Event e=Event::None,bool request=false){return Result{e,t.ownsGrip,request,e==Event::Ejected};};
    auto enter=[&](Phase p,Event e){t.phase=p;t.since=in.nowMs;return out(e);};
    const auto& g=c.gesture;
    if(t.phase!=Phase::Idle&&t.phase!=Phase::Cooldown&&in.capacity!=t.startCapacity)
        return enter(Phase::Cooldown,Event::Cancelled);
    if(t.phase==Phase::WaitingNative){
        const int gain=in.clip-t.startClip;
        if(gain>0&&in.reserve==t.startReserve-gain&&(!f.nativePermitRequired||f.nativePermitConsumed))
            return enter(Phase::Cooldown,Event::Acknowledged);
        if(in.nowMs-t.since>=g.ackMs)return enter(Phase::Cooldown,Event::Rejected);
        return out();
    }
    if(t.phase==Phase::Cooldown){
        if(!in.grip&&in.nowMs-t.since>=g.cooldownMs)t.phase=Phase::Idle;
        return out();
    }
    if(t.phase!=Phase::Idle&&(in.nowMs-t.since>=g.timeoutMs||
       in.clip!=t.startClip||in.reserve!=t.startReserve))return enter(Phase::Cooldown,Event::Cancelled);
    if(t.phase==Phase::Carrying&&(in.handBusy||in.supportLatched))
        return enter(Phase::Cooldown,Event::Cancelled);
    switch(t.phase){
    case Phase::Idle:
        // Keep the prior native-transfer eligibility until per-weapon ammo
        // semantics are proven. No fake native clip=0 or discarded-round writes.
        if(eject&&in.reserve>0&&in.clip<in.capacity){
            t.bodyYaw=std::isfinite(f.bodyYaw)?f.bodyYaw:0;
            t.startClip=in.clip;t.startReserve=in.reserve;t.startCapacity=in.capacity;
            return enter(Phase::NeedAmmo,Event::Ejected);
        }break;
    case Phase::NeedAmmo:
        // A fresh, unowned squeeze may arrive at either shoulder region within
        // the profile's bounded reach interval. Never inherit support/menu grip.
        if(!in.grip||in.handBusy||in.supportLatched||in.knifeNear)t.reachUntil=0;
        else if(gripEdge&&(f.kind==Kind::Valkyrie||f.kind==Kind::Volk||f.kind==Kind::Seeker)&&c.gripReachMs)
            t.reachUntil=in.nowMs+c.gripReachMs;
        if((gripEdge||(in.grip&&t.reachUntil&&in.nowMs<=t.reachUntil))&&
           !t.ownsGrip&&!in.handBusy&&!in.supportLatched&&!in.knifeNear&&
           ShoulderDistance(in.hand,in.pouch,t.bodyYaw,c.shoulder)<=g.pouchRadius){
            t.reachUntil=0;t.ownsGrip=true;return enter(Phase::Carrying,Event::GrabFresh);
        }break;
    case Phase::Carrying:
        if(f.kind==Kind::Seeker){
            if(!f.seekerPath)t.approachSeen=false;
            else if(f.seekerApproach)t.approachSeen=true;
        }
        if(gripRelease){
            const bool measured=(f.kind==Kind::Kasimov||f.kind==Kind::Seeker)&&f.measuredInsertion;
            const auto point=measured?f.insertionPoint:in.hand;
            const bool rear=f.kind!=Kind::Seeker||(t.approachSeen&&f.seekerNear);
            if(rear&&(!measured||f.insertionValid)&&centurionreload::Near(point,in.socket,g.socketRadius)){
                t.phase=Phase::WaitingNative;t.since=in.nowMs;return out(Event::Inserted,true);
            }
            return enter(Phase::NeedAmmo,Event::Discarded);
        }break;
    default:break;
    }
    return out();
}
} // namespace weaponreloadproto
