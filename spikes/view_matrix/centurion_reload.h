#pragma once
#include "hip_knife.h"

// Physical gesture prototype, NOT an inventory implementation. Native UE3 owns
// all ammunition and animation. No UObject writes, allocations or clocks here.
namespace centurionreload {
using hipknife::V; using hipknife::Pose;
struct Config {
    bool cylinderGestures=false; // optional circle/open/dump/insert/close path
    bool circleOpens=false; // separate default-OFF tracked-cylinder experiment
    bool upwardEjection=false; // physical-polish opt-in; old route unchanged
    // Physical left shoulder, tracking metres. Yaw only: usable seated and
    // looking down, separate from both existing hip holsters. Old belt INI
    // keys intentionally do not move this anchor back into the knife zone.
    float shoulderLeft=.22f,shoulderDown=.12f,shoulderBack=.18f,pouchRadius=.13f;
    V socketLocal{-.035f,-.035f,-.08f}; // authored calibration, NOT a UE3 offset
    float socketRadius=.075f,pullDistance=.15f,insertRadius=.16f;
    std::uint64_t timeoutMs=15000,ackMs=4000,cooldownMs=500,grabWindowMs=650;
};
inline float ShoulderLeftMetres(int centimetres){
    return std::fmin(.40f,std::fmax(.15f,std::fabs(float(centimetres)*.01f)));
}
inline bool HeadCentre(V leftEye,V rightEye,V& out){
    if(!hipknife::Finite(leftEye)||!hipknife::Finite(rightEye))return false;
    const float separation=hipknife::Length(rightEye-leftEye);
    if(separation<.005f||separation>.20f)return false;
    out=(leftEye+rightEye)*.5f;return true;
}
inline Pose Pouch(V head,float yaw,const Config& c={}) {
    const hipknife::Q q{0,std::sin(yaw*.5f),0,std::cos(yaw*.5f)};
    return {head+hipknife::Rotate(q,{-c.shoulderLeft,-c.shoulderDown,c.shoulderBack}),q};
}
inline bool Near(V a,V b,float r){return hipknife::Finite(a)&&hipknife::Finite(b)&&
    r>0&&hipknife::Length(a-b)<=r;}
enum class Phase {Idle,Removing,Discard,NeedAmmo,Carrying,WaitingNative,Cooldown,Unlocked,Open,Loaded};
enum class Event {None,GrabOld,Removed,Discarded,GrabFresh,Inserted,Acknowledged,Rejected,Cancelled,Unlocked,Opened,Dumped,Loaded};
inline const char* Name(Event e){switch(e){
    case Event::GrabOld:return "grab-old";case Event::Removed:return "removed";
    case Event::Discarded:return "discarded";case Event::GrabFresh:return "grab-fresh";
    case Event::Inserted:return "inserted/request";case Event::Acknowledged:return "native-ammo-ack";
    case Event::Rejected:return "native-timeout";case Event::Cancelled:return "cancelled";
    case Event::Unlocked:return "cylinder-unlocked";case Event::Opened:return "cylinder-opened";
    case Event::Dumped:return "cylinder-dumped";case Event::Loaded:return "cylinder-loaded";
    default:return "none";}}
struct Frame {
    bool enabled=false,context=false,tracked=false,supported=false,grip=false,firing=false;
    bool handBusy=false,supportLatched=false,knifeNear=false,ammoFresh=false;
    bool releaseCylinder=false;
    bool socketFresh=true; // legacy authored socket; tracked experiment requires render witness
    std::uint32_t pawn=0,weapon=0;
    int clip=-1,reserve=-1,capacity=-1;
    V hand{},socket{},pouch{};
    V insertOffset{}; // carried mesh centre relative to controller; legacy zero
    Pose gun{};
    float bodyYaw=0; // current tracking yaw at gesture start, not recenter yaw
    std::uint64_t nowMs=0;
};
struct State {
    Phase phase=Phase::Idle;
    bool priorGrip=true,ownsGrip=false;
    std::uint64_t gripPressedMs=0;
    bool pendingGrip=false;
    Pose stageGun{};
    float bodyYaw=0;
    std::uint64_t downSince=0;
    bool downSample=false;
    std::uint32_t pawn=0,weapon=0;
    int startClip=-1,startReserve=-1,startCapacity=-1;
    std::uint64_t since=0,lastMs=0;
};
struct Result {Event event=Event::None;bool reserveHand=false,requestNative=false;};
// Read-only HUD observations are not authoritative inventory. Bind each sample
// to its producers and invalidate it across context gaps; never use old counts
// to start or acknowledge a transaction after a pause/swap/failed read.
struct AmmoObservation {
    std::uint32_t pawn=0,weapon=0,hud=0;
    std::uint64_t sampledMs=0;
    int counts[3]{-1,-1,-1};
    bool present=false;
    void Clear(){*this={};}
    void Capture(std::uint32_t p,std::uint32_t w,std::uint32_t h,
                 const int (&values)[3],std::uint64_t now){
        pawn=p;weapon=w;hud=h;sampledMs=now;present=true;
        for(int i=0;i<3;++i)counts[i]=values[i];
    }
    bool Fresh(std::uint32_t p,std::uint32_t w,std::uint32_t h,std::uint64_t now)const{
        return present&&p&&w&&h&&p==pawn&&w==weapon&&h==hud&&
            now>=sampledMs&&now-sampledMs<=250;
    }
};
inline bool AmmoCountsValid(int clip,int reserve,int capacity){return clip>=0&&reserve>=0&&capacity>0&&
    clip<=capacity&&capacity<=1000&&reserve<=1000000;}
inline bool AmmoValid(const Frame& f){return AmmoCountsValid(f.clip,f.reserve,f.capacity);}
// Reseat policy for a stuck HUD latch. The finder keeps the first readable
// deriving instance, which can be a never-populated zombie while the live HUD
// allocates later (same class, same offsets, garbage fields). Only a
// persistently-invalid ELIGIBLE streak may ask for a reseat to a DIFFERENT
// instance; valid reads, ineligible gaps and identity changes reset it.
// Pure policy, no D3D/UE3 calls (unit-tested).
struct HudReseatPolicy {
    unsigned bad=0,reseats=0;
    std::uint64_t lastReseatMs=0;
    static constexpr unsigned kTriggerSamples=50; // ~5 s at the 10 Hz sampler
    static constexpr std::uint64_t kCooldownMs=5000;
    static constexpr unsigned kMaxReseatsPerEpoch=6;
    void NoteIneligible(){bad=0;} // menu/load gaps never accumulate
    void ResetEpoch(){bad=0;reseats=0;} // valid reads + identity changes
    // Eligible sample only. Returns true once when a reseat is due.
    bool NoteSample(bool valid,std::uint64_t nowMs){
        if(valid){ResetEpoch();return false;}
        if(reseats>=kMaxReseatsPerEpoch)return false;
        if(++bad<kTriggerSamples)return false;
        if(nowMs<lastReseatMs||nowMs-lastReseatMs<kCooldownMs)return false;
        lastReseatMs=nowMs;bad=0;++reseats;return true;
    }
};
inline bool Active(Phase p){return p!=Phase::Idle&&p!=Phase::Cooldown;}
inline float AnchorYaw(const State& s,float currentYaw){return Active(s.phase)?s.bodyYaw:currentYaw;}
inline bool Flicked(const Pose& a,const Pose& b){
    // Controller-local lateral displacement or roll; deliberately NOT forward
    // translation (normal aiming) or controller pitch alone.
    const V delta=hipknife::Rotate({-a.q.x,-a.q.y,-a.q.z,a.q.w},b.p-a.p);
    const auto relative=hipknife::Multiply({-a.q.x,-a.q.y,-a.q.z,a.q.w},b.q);
    return std::fabs(delta.x)>=.07f||std::fabs(relative.z)>=.24f;
}
inline Result Update(State& s,const Frame& f,const Config& c={}){
    const bool edge=f.grip&&!s.priorGrip;
    const bool release=!f.grip&&s.priorGrip;
    s.priorGrip=f.grip;
    if(edge){s.gripPressedMs=f.nowMs;s.pendingGrip=true;}
    if(!f.grip)s.pendingGrip=false;
    const bool grab=f.grip&&s.pendingGrip&&f.nowMs>=s.gripPressedMs&&
        f.nowMs-s.gripPressedMs<=c.grabWindowMs;
    // A squeeze just outside the cylinder used to latch support first and
    // permanently exclude reload. Allow this SAME short acquisition gesture
    // to reach the precise reload socket; never steal a long-held support
    // grip, a held knife, or a squeeze inherited from a menu/tracking gap.
    const bool oldIntent=s.phase==Phase::Idle&&!s.ownsGrip&&grab&&!f.knifeNear&&
        f.reserve>0&&f.clip<f.capacity&&Near(f.hand,f.socket,c.socketRadius);
    const bool awaitingGrip=(s.phase==Phase::Idle||s.phase==Phase::NeedAmmo||
        (c.cylinderGestures&&!s.ownsGrip))&&!s.ownsGrip;
    const bool changed=(s.weapon&&s.weapon!=f.weapon)||(s.pawn&&s.pawn!=f.pawn);
    const bool valid=f.enabled&&f.context&&f.tracked&&f.supported&&f.pawn&&f.weapon&&
        f.ammoFresh&&AmmoValid(f)&&hipknife::Finite(f.hand)&&hipknife::Finite(f.socket)&&hipknife::Finite(f.pouch)&&hipknife::Finite(f.insertOffset)&&
        !f.handBusy&&(!f.supportLatched||awaitingGrip)&&!f.firing&&!changed&&f.nowMs>=s.lastMs&&
        std::isfinite(f.bodyYaw)&&(!c.cylinderGestures||(hipknife::Finite(f.gun.p)&&hipknife::Valid(f.gun.q)));
    s.lastMs=f.nowMs;
    if(!valid){
        const bool had=s.phase!=Phase::Idle;
        // Hold ownership until physical release, including a tracking/menu
        // cancellation. Otherwise the same squeeze can become TMD/Use.
        const bool blocked=s.ownsGrip&&f.grip;
        s={};s.priorGrip=f.grip;s.ownsGrip=blocked;s.lastMs=f.nowMs;
        return {had?Event::Cancelled:Event::None,blocked,false};
    }
    s.pawn=f.pawn;s.weapon=f.weapon;
    if(!f.grip)s.ownsGrip=false;
    auto out=[&](Event e=Event::None,bool request=false){return Result{e,s.ownsGrip,request};};
    auto enter=[&](Phase p,Event e){s.phase=p;s.since=f.nowMs;return out(e);};
    if(s.phase!=Phase::Idle&&s.phase!=Phase::Cooldown&&f.capacity!=s.startCapacity)
        return enter(Phase::Cooldown,Event::Cancelled);
    if(s.phase==Phase::WaitingNative){
        // Exact conservation for a native reload; do NOT credit a pickup or
        // merely a callback. A late HUD update that fails this remains unknown.
        const int gained=f.clip-s.startClip;
        if(gained>0&&f.reserve==s.startReserve-gained)
            return enter(Phase::Cooldown,Event::Acknowledged);
        if(f.nowMs-s.since>=c.ackMs)return enter(Phase::Cooldown,Event::Rejected);
        return out();
    }
    if(s.phase==Phase::Cooldown){
        if(!f.grip&&f.nowMs-s.since>=c.cooldownMs)s.phase=Phase::Idle;
        return out();
    }
    if(s.phase!=Phase::Idle&&f.nowMs-s.since>=c.timeoutMs)
        return enter(Phase::Cooldown,Event::Cancelled);
    // A native reload/pickup/shot during the gesture invalidates its snapshot.
    if(s.phase!=Phase::Idle&&(f.clip!=s.startClip||f.reserve!=s.startReserve))
        return enter(Phase::Cooldown,Event::Cancelled);
    switch(s.phase){
    case Phase::Idle:
        if(c.cylinderGestures){
            if(f.releaseCylinder&&f.reserve>0&&f.clip<f.capacity){
                s.stageGun=f.gun;s.bodyYaw=f.bodyYaw;s.startClip=f.clip;s.startReserve=f.reserve;s.startCapacity=f.capacity;
                return c.circleOpens?enter(Phase::Open,Event::Opened):enter(Phase::Unlocked,Event::Unlocked);
            }
        }else if(oldIntent){
            s.pendingGrip=false;s.ownsGrip=true;s.bodyYaw=f.bodyYaw;s.startClip=f.clip;s.startReserve=f.reserve;s.startCapacity=f.capacity;
            return enter(Phase::Removing,Event::GrabOld);
        }break;
    case Phase::Removing:
        if(!f.grip)return enter(Phase::Cooldown,Event::Cancelled);
        if(!Near(f.hand,f.socket,c.pullDistance))return enter(Phase::Discard,Event::Removed);
        break;
    case Phase::Discard:
        if(release)return enter(Phase::NeedAmmo,Event::Discarded);
        break;
    case Phase::NeedAmmo:
        if(grab&&!f.knifeNear&&Near(f.hand,f.pouch,c.pouchRadius)){
            s.pendingGrip=false;s.ownsGrip=true;return enter(Phase::Carrying,Event::GrabFresh);
        }break;
    case Phase::Carrying:
        if(release){
            if(f.socketFresh&&Near(f.hand+f.insertOffset,f.socket,c.insertRadius)){
                if(c.cylinderGestures){s.stageGun=f.gun;return enter(Phase::Loaded,Event::Loaded);}
                s.phase=Phase::WaitingNative;s.since=f.nowMs;
                return out(Event::Inserted,true);
            }
            return enter(Phase::NeedAmmo,Event::Discarded);
        }break;
    case Phase::Unlocked:
        if(Flicked(s.stageGun,f.gun))return enter(Phase::Open,Event::Opened);
        break;
    case Phase::Open:{
        // Legacy downward / opt-in physical upward muzzle for 100ms, not noise.
        // Gesture observation only; no fabricated inventory/animation writes.
        const V forward=hipknife::Rotate(f.gun.q,{0,0,-1});
        if(c.upwardEjection?forward.y>.55f:forward.y<-.55f){
            if(!s.downSample){s.downSince=f.nowMs;s.downSample=true;}
            if(f.nowMs-s.downSince>=100)return enter(Phase::NeedAmmo,Event::Dumped);
        }else s.downSample=false;
        break;}
    case Phase::Loaded:
        if(f.nowMs-s.since>=100&&hipknife::Rotate(f.gun.q,{0,0,-1}).y>-.25f&&Flicked(s.stageGun,f.gun)){
            s.phase=Phase::WaitingNative;s.since=f.nowMs;return out(Event::Inserted,true);
        }break;
    default:break;
    }
    return out();
}
// A bounded ordinary XInput Use/reload tap. It is deliberately NOT a direct
// engine Reload call. Shared Use semantics require MAIN scene verification.
struct NativeTap {
    std::uint64_t end=0;bool gap=false;
    void Cancel(){end=0;gap=false;}
    bool Update(bool valid,bool request,std::uint64_t now){
        if(!valid){Cancel();return false;}
        if(request){end=now+120;gap=true;return false;}
        if(gap){gap=false;return false;} // clean low edge before synthetic tap
        if(end&&now<end)return true;
        end=0;return false;
    }
};
} // namespace centurionreload
