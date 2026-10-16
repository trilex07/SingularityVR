#pragma once
#include "centurion_reload.h"
#include "weapon_reload_profiles.h"
#include <cstring>

// CPU-only next-weapon prototypes. NOT connected to OpenXR/XInput/UE3.
// Reuse transaction safety, not the Centurion cylinder/socket or native tap.
namespace weaponreloadproto {
struct Shoulder {float left=.22f,down=.12f,back=.18f;};
inline hipknife::Pose Supply(hipknife::V headCentre,float latchedPhysicalYaw,const Shoulder& c){
    const hipknife::Q q{0,std::sin(latchedPhysicalYaw*.5f),0,std::cos(latchedPhysicalYaw*.5f)};
    return {headCentre+hipknife::Rotate(q,{-c.left,-c.down,c.back}),q};
}
struct Calibration {
    bool ready=false;
    std::uint32_t revision=0; // caller increments on any calibration change
    centurionreload::Config gesture{}; // INERT until explicitly calibrated
    Shoulder shoulder{}; // physical left/back; caller must use Supply, not old belt Pouch
};
inline bool Valid(const Calibration& c){
    const auto& g=c.gesture;
    return c.ready&&c.revision&&hipknife::Finite(g.socketLocal)&&
        hipknife::Length(g.socketLocal)<=1.f&&std::isfinite(c.shoulder.left)&&
        std::isfinite(c.shoulder.down)&&std::isfinite(c.shoulder.back)&&
        c.shoulder.left>=.15f&&c.shoulder.left<=.40f&&c.shoulder.down>=0&&c.shoulder.down<=.25f&&
        c.shoulder.back>=.10f&&c.shoulder.back<=.35f&&
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
    centurionreload::Frame gesture{};
};
struct State {
    Kind profile=Kind::None;
    std::uint32_t calibrationRevision=0;
    bool haveCalibration=false;
    centurionreload::Config priorCalibration{};
    Shoulder priorShoulder{};
    centurionreload::State transaction{};
};
struct Result {
    centurionreload::Event event=centurionreload::Event::None;
    bool reservePhysicalLeft=false;
    bool reloadIntent=false; // abstract intent ONLY, never a native call
};
inline Result Update(State& s,const Frame& f,const Calibration& c){
    const auto* profile=Find(f.kind);
    const bool identity=profile&&f.liveIdentityVerified&&f.objectName&&
        !std::strcmp(f.objectName,profile->objectName)&&f.meshWedges==profile->meshWedges;
    const bool calibrated=Valid(c);
    const bool changed=(s.profile!=Kind::None&&s.profile!=f.kind)||
        (s.haveCalibration&&(s.calibrationRevision!=c.revision||!Same(s.priorCalibration,c.gesture)||
            s.priorShoulder.left!=c.shoulder.left||s.priorShoulder.down!=c.shoulder.down||s.priorShoulder.back!=c.shoulder.back));
    auto input=f.gesture;
    input.enabled=f.enabled&&identity&&f.aimHand==1&&calibrated&&!changed&&!input.supportLatched;
    // Copy only reviewed shared fields. New Centurion cylinder/layout settings
    // must not silently change these magazine-only prototypes when integrated.
    centurionreload::Config gesture{};
    gesture.socketLocal=c.gesture.socketLocal;gesture.socketRadius=c.gesture.socketRadius;
    gesture.pullDistance=c.gesture.pullDistance;gesture.pouchRadius=c.gesture.pouchRadius;
    gesture.timeoutMs=c.gesture.timeoutMs;gesture.ackMs=c.gesture.ackMs;gesture.cooldownMs=c.gesture.cooldownMs;
    gesture.insertRadius=c.gesture.socketRadius;gesture.grabWindowMs=0;
    const auto result=centurionreload::Update(s.transaction,input,gesture);
    s.profile=identity?f.kind:Kind::None;
    s.calibrationRevision=calibrated?c.revision:0;s.haveCalibration=calibrated;
    if(calibrated){s.priorCalibration=c.gesture;s.priorShoulder=c.shoulder;}
    return {result.event,result.reserveHand,result.requestNative};
}
} // namespace weaponreloadproto
