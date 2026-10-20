#include "../centurion_clip_preview.h"
#include "../pose_conversion.h"
#include <cassert>
#include <cstdio>
#include <limits>
using namespace centurionreload;
int main(){
    Config c;unsigned checked=0;
    // More forgiving supply pickup, not a wider insertion/holster region.
    assert(std::fabs(c.pouchRadius-.17f)<1e-6f);
    assert(Near({.16f,0,0},{},c.pouchRadius));
    assert(!Near({.18f,0,0},{},c.pouchRadius));
    assert(std::fabs(c.insertRadius-.16f)<1e-6f);
    for(int raw:{-22,22,-2000,2000,0}){
        c.shoulderLeft=ShoulderLeftMetres(raw);
        assert(c.shoulderLeft>=.15f&&c.shoulderLeft<=.40f);
        if(raw==-22||raw==22)assert(std::fabs(c.shoulderLeft-.22f)<1e-6f);
        for(int degrees=-180;degrees<=180;++degrees)for(int pitch:{-80,-40,0,40,80})for(int roll:{-120,0,120}){
            const auto q=centurionclip::Degrees(float(pitch),float(degrees),float(roll));
            // Actual producer used by g_headYawRad, not a second guessed sign.
            const auto e=vrpose::DecomposeOpenXrYXZ({q.x,q.y,q.z,q.w});
            const float radians=float(degrees)*3.14159265359f/180.f;
            const V left{-std::cos(radians),0,std::sin(radians)};
            const V back{std::sin(radians),0,std::cos(radians)};
            const V head{1.2f,.35f,-2.1f}; // user can test seated on the floor
            V centre{};
            const V separation=hipknife::Rotate(q,{.064f,0,0});
            assert(HeadCentre(head-separation*.5f,head+separation*.5f,centre));
            assert(hipknife::Length(centre-head)<1e-6f);
            State s;s.phase=Phase::Unlocked;s.bodyYaw=e.yawY;
            const auto source=Pouch(centre,AnchorYaw(s,e.yawY+1.2f),c);
            const auto delta=source.p-head;
            assert(std::fabs(hipknife::Dot(delta,left)-c.shoulderLeft)<1e-5f);
            assert(std::fabs(hipknife::Dot(delta,back)-c.shoulderBack)<1e-5f);
            assert(std::fabs(delta.y+c.shoulderDown)<1e-6f);
            assert(!Near(head-left*c.shoulderLeft+back*c.shoulderBack+V{0,-c.shoulderDown,0},source.p,c.pouchRadius));
            // Exact target also feeds the rendered stock mesh, with no hidden
            // hand-mount offset. Knife at the belt remains a different region.
            centurionclip::Snapshot snapshot;snapshot.phase=Phase::NeedAmmo;
            centurionclip::Config clip;clip.offset={.1f,.2f,-.1f};hipknife::Pose pose;
            assert(centurionclip::PreviewPose(snapshot,source,clip,pose));
            assert(hipknife::Length(pose.p-source.p)<1e-7f);
            assert(Near(source.p,pose.p,c.pouchRadius));
            ++checked;
        }
    }
    for(float ipd:{.04f,.064f,.08f})for(float height:{.3f,1.f,1.8f}){
        V centre{};assert(HeadCentre({-ipd*.5f,height,0},{ipd*.5f,height,0},centre));
        assert(centre.x==0&&centre.y==height&&centre.z==0);
    }
    V unchanged{1,2,3};assert(!HeadCentre({},{},unchanged)&&unchanged.x==1);
    assert(!HeadCentre({0,0,0},{.21f,0,0},unchanged));
    assert(!HeadCentre({std::numeric_limits<float>::quiet_NaN(),0,0},{.06f,0,0},unchanged));
    std::printf("L1 shoulder PASS: %u full producer/basis/IPD/preview checks; latched physical left/back, invalid eye refusal\n",checked);
}
