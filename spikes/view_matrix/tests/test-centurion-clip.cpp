#include "../centurion_clip_preview.h"
#include <cassert>
#include <limits>
#include <cstdio>
using namespace centurionclip;
struct FakeGuard {
    bool valid=true,restoreOk=true;
    unsigned restores=0;
    bool Restore(){assert(valid);valid=false;++restores;return restoreOk;}
};
static void TestPresentationSafety(){
    PresentationSafety safety;
    unsigned draws=0;
    auto draw=[&](){++draws;return true;};
    FakeGuard normal;
    assert(safety.Render(normal,draw)==DrawResult::Success);
    assert(draws==1&&normal.restores==1&&safety.Allowed());
    // Early draw failure still restores; a healthy outer restore allows retry.
    for(unsigned failedStage=0;failedStage<8;++failedStage){
        FakeGuard guard;
        unsigned stages=0;
        auto partial=[&](){for(unsigned i=0;i<8;++i){++stages;if(i==failedStage)return false;}return true;};
        assert(safety.Render(guard,partial)==DrawResult::DrawFailed);
        assert(stages==failedStage+1&&guard.restores==1&&safety.Allowed());
    }
    FakeGuard restoreFault;restoreFault.restoreOk=false;
    assert(safety.Render(restoreFault,draw)==DrawResult::RestoreFailed);
    assert(restoreFault.restores==1&&!safety.Allowed());
    const unsigned before=draws;
    for(unsigned frame=0;frame<100000;++frame){
        FakeGuard guard;
        assert(safety.Render(guard,draw)==DrawResult::Disabled);
        assert(guard.restores==0&&draws==before&&!safety.Allowed());
    }
    // Reset is the sole recovery route, not a successful later input frame.
    safety.Reset();FakeGuard recovered;
    assert(safety.Render(recovered,draw)==DrawResult::Success&&safety.Allowed());
    FakeGuard captureFault;captureFault.valid=false;
    const unsigned beforeCapture=draws;
    assert(safety.Render(captureFault,draw)==DrawResult::CaptureFailed);
    assert(draws==beforeCapture&&captureFault.restores==0&&!safety.Allowed());
    safety.Reset();FakeGuard both;both.restoreOk=false;
    assert(safety.Render(both,[](){return false;})==DrawResult::RestoreFailed);
    assert(both.restores==1&&!safety.Allowed());
}
int main(){
    TestPresentationSafety();
    float size=0;
    assert(NativeSize(52.5f,100.f,size)&&std::fabs(size-1.f)<1e-6f);
    assert(NativeSize(65.625f,75.f,size)&&std::fabs(size-.6f)<1e-6f);
    // Compare every native UU point with the exported metre point under the
    // same world/gun transforms; no arbitrary independent ammo-size tuning.
    for(int world=50;world<=200;world+=5)for(int gun=25;gun<=400;gun+=5){
        const float uu=52.5f*100.f/(float)world;
        assert(NativeSize(uu,(float)gun,size));
        for(float localUU:{-5.2f,0.f,3.3f})
            assert(std::fabs((localUU/52.5f)*size-(localUU/uu)*(float(gun)/100.f))<1e-6f);
    }
    for(float bad:{0.f,-1.f,1001.f,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()}){
        size=7;assert(!NativeSize(bad,75,size)&&size==7);
    }
    for(float bad:{0.f,24.f,401.f,std::numeric_limits<float>::quiet_NaN()}){
        size=7;assert(!NativeSize(65.625f,bad,size)&&size==7);
    }
    Snapshot s;s.visible=true;s.phase=centurionreload::Phase::Carrying;
    s.pawn=20;s.weapon=10;s.sampledMs=1000;s.hand={{.2f,1,-.5f},{}};
    assert(Eligible(s,true,20,10,1000)&&Eligible(s,true,20,10,1100));
    assert(!Eligible(s,false,20,10,1000));
    assert(!Eligible(s,true,21,10,1000)&&!Eligible(s,true,20,11,1000));
    assert(!Eligible(s,true,20,10,1101)&&!Eligible(s,true,20,10,999));
    for(auto phase:{centurionreload::Phase::Idle,centurionreload::Phase::Removing,
        centurionreload::Phase::Discard,centurionreload::Phase::Unlocked,
        centurionreload::Phase::Open,centurionreload::Phase::Loaded,
        centurionreload::Phase::WaitingNative,centurionreload::Phase::Cooldown}){
        s.phase=phase;assert(!Eligible(s,true,20,10,1000));
    }
    s.phase=centurionreload::Phase::Carrying;s.visible=false;assert(!Eligible(s,true,20,10,1000));
    s.visible=true;Pose pose;Config cfg;assert(Mount(s.hand,cfg,pose));
    assert(std::fabs(pose.p.x-.2f)<1e-6f&&std::fabs(pose.p.z+.58f)<1e-6f);
    s.phase=centurionreload::Phase::NeedAmmo;
    assert(Eligible(s,true,20,10,1000));
    const Pose stock{{-.22f,1.48f,.18f},Degrees(0,30,0)};
    assert(PreviewPose(s,stock,cfg,pose));
    assert(hipknife::Length(pose.p-stock.p)==0); // not displaced by hand offset
    s.phase=centurionreload::Phase::Carrying;
    assert(PreviewPose(s,stock,cfg,pose)&&std::fabs(pose.p.z+.58f)<1e-6f);
    s.phase=centurionreload::Phase::Loaded;assert(!PreviewPose(s,stock,cfg,pose));
    s.phase=centurionreload::Phase::Carrying;
    // Authored offset rotates with the controller, never the head/camera.
    s.hand.q=Degrees(0,90,0);assert(Mount(s.hand,cfg,pose));
    assert(std::fabs(pose.p.x-.12f)<1e-6f&&std::fabs(pose.p.z+.5f)<1e-6f);
    cfg.rotation=Degrees(30,-15,50);assert(Mount(s.hand,cfg,pose)&&hipknife::Valid(pose.q));
    // Same symmetric projection contract as existing knife/menu overlays.
    // CPU consistency is not optical alignment or submitted-pixel proof.
    for(int eye=0;eye<2;++eye){
        Pose camera{{eye?.032f:-.032f,1.6f,0},Degrees(5,-10,3)};
        const V point{.1f,1.2f,-.8f};const auto local=hipknife::EyeLocal(point,camera);
        float projection[4][4];assert(hipknife::Projection(1.1f,1.2f,projection));
        const float clipX=local.x*projection[0][0],clipY=local.y*projection[1][1],clipW=-local.z;
        uipointer::ScreenPoint screen;
        assert(uipointer::ProjectPoint(point,camera.p,hipknife::Rotate(camera.q,{0,0,-1}),
            hipknife::Rotate(camera.q,{1,0,0}),hipknife::Rotate(camera.q,{0,1,0}),1.1f,1.2f,screen));
        assert(std::fabs(screen.u-(.5f+.5f*clipX/clipW))<1e-6f);
        assert(std::fabs(screen.v-(.5f-.5f*clipY/clipW))<1e-6f);
    }
    cfg.offset={.31f,0,0};assert(!Mount(s.hand,cfg,pose));
    cfg={};s.hand.q={0,0,0,2};assert(!Mount(s.hand,cfg,pose)&&!Current(s,20,10,1000));
    s.hand.q={};s.hand.p.x=std::numeric_limits<float>::quiet_NaN();
    assert(!Mount(s.hand,cfg,pose)&&!Current(s,20,10,1000));
    for(int p=-180;p<=180;p+=10)for(int y=-180;y<=180;y+=10)for(int r=-180;r<=180;r+=10)
        assert(hipknife::Valid(Degrees((float)p,(float)y,(float)r)));
    std::puts("centurion clip L1 PASS: default OFF/context/phase/freshness/identity/controller mount/50653 rotations + presentation fault latch/100000 disabled frames");
}
