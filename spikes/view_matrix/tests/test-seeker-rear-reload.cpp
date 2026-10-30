#include "../weapon_reload_carry.h"
#include "../native_refill_only.h"
#include <cassert>
#include <cstdio>

using namespace weaponreload;
using namespace weaponreloadproto;
using V=hipknife::V;
static armbind::Mat Identity(){armbind::Mat m{};for(unsigned i=0;i<4;++i)m.m[i][i]=1;return m;}
static armbind::Mat At(V p){auto m=Identity();m.m[3][0]=p.x;m.m[3][1]=p.y;m.m[3][2]=p.z;return m;}
static bool Route(const armbind::Mat& mag,const armbind::Mat& socket,V point,bool& path,bool& approach,bool& near){
    return SeekerCorridor(mag,socket,point,path,approach,near);
}
struct Fixture {
    State state{};Frame f{};Calibration cfg{};V socket{0,1,0};V axis{};
    Fixture(){
        assert(SeekerRearAxis(At(socket),axis));
        f.kind=Kind::Seeker;f.enabled=f.liveIdentityVerified=f.measuredInsertion=true;
        f.objectName="ARSteerable";f.meshWedges=12444;f.nativePermitRequired=true;
        auto& in=f.gesture;in.context=in.tracked=in.supported=in.ammoFresh=true;
        in.pawn=7;in.weapon=9;in.clip=1;in.reserve=5;in.capacity=4;
        in.socket=socket;in.hand=socket;in.pouch=Supply({0,1.1f,0},0,cfg.shoulder).p;
        cfg.ready=true;cfg.revision=1;cfg.gesture.socketRadius=.07f;
        cfg.gesture.pouchRadius=.18f;cfg.shoulder.leftExtension=.18f;
    }
    Result Tick(bool grip,bool circle=false){
        auto& in=f.gesture;in.nowMs+=20;in.grip=grip;f.magazineReleaseDown=circle;
        return Update(state,f,cfg);
    }
    void Start(){Tick(false);assert(Tick(false,true).event==Event::Ejected);
        f.gesture.hand=f.gesture.pouch;assert(Tick(true).event==Event::GrabFresh);}
    void Carry(float depth){
        const auto mag=At(socket+axis*depth);bool path=false,approach=false,near=false;
        assert(Route(mag,At(socket),socket,path,approach,near));
        f.insertionValid=true;f.insertionPoint=Transform({},mag);
        f.seekerPath=path;f.seekerApproach=approach;f.seekerNear=near;
        Tick(true);
    }
};
struct Native {
    nativerefill::Counts counts{1,5,4};int calls=0;
    nativerefill::Counts Read(){return counts;}
    void Refill(){++calls;counts={4,2,4};}
};
int main(){
    auto seated=At({0,1,0});V rear;assert(SeekerRearAxis(seated,rear));
    bool path=false,approach=false,near=false;
    assert(Route(At(V{0,1,0}+rear*.20f),seated,{0,1,0},path,approach,near)&&path&&approach&&!near);
    assert(Route(At(V{0,1,0}-rear*.20f),seated,{0,1,0},path,approach,near)&&!path&&!approach&&!near);
    assert(Route(At(V{.2f,1,0}),seated,{0,1,0},path,approach,near)&&!path&&!approach&&!near);
    auto rotated=At(V{0,1,0}+rear*.20f);rotated.m[0][0]=0;rotated.m[0][2]=1;
    rotated.m[2][0]=-1;rotated.m[2][2]=0;
    assert(!Route(rotated,seated,{0,1,0},path,approach,near));
    auto ejected=seated;assert(SeekerEject(ejected,.25f));
    assert(hipknife::Length(Transform({},ejected)-(V{0,1,0}+rear*.14f))<.00001f);
    Fixture successful;successful.Start();successful.Carry(.20f);successful.Carry(.025f);
    assert(successful.Tick(false).event==Event::Inserted);
    Native native;assert(nativerefill::Commit({1,5,4},native)==nativerefill::Result::Conserved);
    assert(native.calls==1);successful.f.nativePermitConsumed=true;
    successful.f.gesture.clip=native.counts.clip;successful.f.gesture.reserve=native.counts.reserve;
    assert(successful.Tick(false).event==Event::Acknowledged);
    assert(successful.Tick(false).event!=Event::Acknowledged);
    Fixture front;front.Start();front.Carry(.025f);
    assert(front.Tick(false).event==Event::Discarded);
    Fixture lost;lost.Start();lost.Carry(.20f);lost.f.gesture.tracked=false;
    assert(lost.Tick(true).event==Event::Cancelled);
    Fixture full;full.f.gesture.clip=4;full.Tick(false);assert(full.Tick(false,true).event==Event::None);
    Fixture empty;empty.f.gesture.reserve=0;empty.Tick(false);assert(empty.Tick(false,true).event==Event::None);
    armbind::Mat mount,model;assert(weaponcarry::Mount(Kind::Seeker,mount));
    auto wrist=Identity();for(unsigned i=0;i<3;++i)wrist.m[i][i]=1.f/52.5f;
    assert(weaponcarry::Matrix(Kind::Seeker,wrist,model));
    assert(std::fabs(mount.m[3][1]+9.05215640f)<.00001f);
    puts("PASS Seeker rear axis/orientation/corridor, source glove mount, single conserved native-refill policy and cancellation (offline only)");
}
