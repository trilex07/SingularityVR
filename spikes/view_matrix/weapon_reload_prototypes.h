#pragma once
#include "centurion_reload.h"
#include <cstring>

// CPU-only next-weapon prototypes. NOT connected to OpenXR/XInput/UE3.
// Reuse transaction safety, not the Centurion cylinder/socket or native tap.
namespace weaponreloadproto {
enum class Kind {None,Valkyrie,Volk,Kasimov};
struct Profile {
    Kind kind;const char* name;const char* objectName;
    long meshWedges;const char* clipParent;
};
inline constexpr Profile profiles[]{
    {Kind::Valkyrie,"AR9 Valkyrie","ARMachineGun",11387,"cog_machinegun"},
    {Kind::Volk,"Volk S4","ARShotgun",8297,"cog_shotgun"},
    {Kind::Kasimov,"Kasimov SNV-E99","ARSniperRifle",11389,"cog_sniper"}
};
inline const Profile* Find(Kind kind){
    for(const auto& p:profiles)if(p.kind==kind)return &p;
    return nullptr;
}
struct Calibration {
    bool ready=false;
    std::uint32_t revision=0; // caller increments on any calibration change
    centurionreload::Config gesture{}; // INERT until explicitly calibrated
};
inline bool Valid(const Calibration& c){
    const auto& g=c.gesture;
    return c.ready&&c.revision&&hipknife::Finite(g.socketLocal)&&
        hipknife::Length(g.socketLocal)<=1.f&&std::isfinite(g.pouchDown)&&
        std::isfinite(g.pouchForward)&&g.pouchDown>=0&&g.pouchDown<=1.f&&
        g.pouchForward>=0&&g.pouchForward<=1.f&&
        g.socketRadius>0&&g.socketRadius<g.pullDistance&&g.pullDistance<=1.f&&
        g.pouchRadius>0&&g.pouchRadius<=.5f&&g.timeoutMs>=100&&g.timeoutMs<=60000&&
        g.ackMs>=100&&g.ackMs<=60000&&g.cooldownMs<=5000;
}
inline bool Same(const centurionreload::Config& a,const centurionreload::Config& b){
    return a.pouchDown==b.pouchDown&&a.pouchForward==b.pouchForward&&a.pouchRadius==b.pouchRadius&&
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
        (s.haveCalibration&&(s.calibrationRevision!=c.revision||!Same(s.priorCalibration,c.gesture)));
    auto input=f.gesture;
    input.enabled=f.enabled&&identity&&f.aimHand==1&&calibrated&&!changed;
    const auto result=centurionreload::Update(s.transaction,input,c.gesture);
    s.profile=identity?f.kind:Kind::None;
    s.calibrationRevision=calibrated?c.revision:0;s.haveCalibration=calibrated;
    if(calibrated)s.priorCalibration=c.gesture;
    return {result.event,result.reserveHand,result.requestNative};
}
} // namespace weaponreloadproto
