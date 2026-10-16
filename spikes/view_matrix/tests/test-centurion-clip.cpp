#include "../centurion_clip_preview.h"
#include <cassert>
#include <limits>
#include <cstdio>
using namespace centurionclip;
int main(){
    Snapshot s;s.visible=true;s.phase=centurionreload::Phase::Carrying;
    s.pawn=20;s.weapon=10;s.sampledMs=1000;s.hand={{.2f,1,-.5f},{}};
    assert(Eligible(s,true,20,10,1000)&&Eligible(s,true,20,10,1100));
    assert(!Eligible(s,false,20,10,1000));
    assert(!Eligible(s,true,21,10,1000)&&!Eligible(s,true,20,11,1000));
    assert(!Eligible(s,true,20,10,1101)&&!Eligible(s,true,20,10,999));
    for(auto phase:{centurionreload::Phase::Idle,centurionreload::Phase::Removing,
        centurionreload::Phase::Discard,centurionreload::Phase::NeedAmmo,
        centurionreload::Phase::WaitingNative,centurionreload::Phase::Cooldown}){
        s.phase=phase;assert(!Eligible(s,true,20,10,1000));
    }
    s.phase=centurionreload::Phase::Carrying;s.visible=false;assert(!Eligible(s,true,20,10,1000));
    s.visible=true;Pose pose;Config cfg;assert(Mount(s.hand,cfg,pose));
    assert(std::fabs(pose.p.x-.2f)<1e-6f&&std::fabs(pose.p.z+.58f)<1e-6f);
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
    std::puts("centurion clip L1 PASS: default OFF/context/phase/freshness/identity/controller mount/50653 rotations");
}
