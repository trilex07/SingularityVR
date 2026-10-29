#define NOMINMAX
#include "../centurion_physical_polish.h"
#include "../centurion_native_reload.h"
#include "../centurion_shells.h"
#include "../centurion_clip_preview.h"
#include "../arm_natural.h"
#include <cassert>
#include <cstdio>
using namespace centurionphysical;
static Mat M(Pose p){Mat m;assert(PoseMatrix(p,m));return m;}
static void Near(const Mat& a,const Mat& b){for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j)assert(std::fabs(a.m[i][j]-b.m[i][j])<.004f);}
int main(){
    unsigned cases=0;
    for(int yaw=-180;yaw<180;yaw+=15)for(int pitch:{-60,0,60})for(float scale:{.5f,1.f,2.f}){
        RightHold hold;float palette[135][4]{};bool right[45]{};
        for(unsigned i=0;i<45;++i){right[i]=i%2==1;armbind::ToGpu(M({{float(i),10,-5},{}}),reinterpret_cast<float(*)[4]>(palette+i*3));}
        const auto original=palette[3][3];
        auto chain=M({{100,-40,80},centurionclip::Degrees(15,-40,3)});
        for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)chain.m[r][c]*=scale;
        Mat tracking;assert(TrackingMap({.1f,.3f,-.2f},.4f,-.8f,52.5f,tracking));
        const Pose aim{{.2f,.1f,-.5f},centurionclip::Degrees(float(pitch),float(yaw),30)};
        assert(!hold.Apply(1,2,3,false,palette,45,right,chain,tracking,aim));
        assert(hold.Apply(1,2,3,true,palette,45,right,chain,tracking,aim));assert(std::fabs(palette[3][3]-original)<.0001f);
        auto moving=M({{-30,70,-125},centurionclip::Degrees(-24,100,15)});
        for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)moving.m[r][c]*=scale;
        const Pose newAim{{-.3f,.2f,-.9f},centurionclip::Degrees(-15,35,50)};
        for(unsigned i=0;i<45;++i)armbind::ToGpu(M({{-40,50,float(i)},centurionclip::Degrees(80,90,40)}),reinterpret_cast<float(*)[4]>(palette+i*3));
        float before[135][4];std::memcpy(before,palette,sizeof(before));
        assert(hold.Apply(1,2,3,false,palette,45,right,moving,tracking,newAim));
        Mat invTracking,invAim;assert(Inverse(tracking,invTracking)&&Inverse(M(newAim),invAim));
        for(unsigned i=0;i<45;++i)if(right[i]){Mat b;assert(armbind::FromGpu(reinterpret_cast<float(*)[4]>(palette+i*3),b));
            Near(Product(Product(Product(b,moving),invTracking),invAim),hold.relative[i]);
        }else assert(!memcmp(palette+i*3,before+i*3,3*4*sizeof(float)));
        Mat wrist=Identity(),grip;float gpu[3][4];armbind::ToGpu(Identity(),gpu);
        assert(Grip(wrist,gpu,moving,tracking,grip));
        Near(grip,Product(Product(PalmMount(),moving),invTracking));
        auto small=grip;for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)small.m[i][j]*=.7f;
        Mat fitted;assert(MatchSize(grip,small,fitted));Near(fitted,small);
        auto invalid=moving;invalid.m[0][0]=NAN;std::memcpy(before,palette,sizeof(before));
        assert(!hold.Apply(1,2,3,false,palette,45,right,invalid,tracking,newAim)&&!memcmp(before,palette,sizeof(before)));
        assert(!hold.Apply(1,2,4,false,palette,45,right,moving,tracking,newAim));++cases;
    }
    // Material-0 TMD wrist: use its actual bind and the final local*ride chain.
    const auto& tmd=armnatural::TmdBind();assert(tmd.ready);
    for(float angle:{-70.f,40.f}) {
        auto skin=M({{2,-3,7},centurionclip::Degrees(angle,20,0)});
        auto local=M({{30,-12,70},centurionclip::Degrees(5,angle,0)});
        auto ride=M({{-9,20,5},centurionclip::Degrees(0,angle,30)});
        Mat tracking;assert(TrackingMap({.1f,.3f,-.2f},.4f,-.8f,52.5f,tracking));
        const auto chain=Product(local,ride);float gpu[3][4];armbind::ToGpu(skin,gpu);
        Mat result,inv;assert(Grip(tmd.bind[9],gpu,chain,tracking,result));assert(Inverse(tracking,inv));
        Near(result,Product(Product(Product(Product(PalmMount(),tmd.bind[9]),skin),chain),inv));
    }
    centurionnative::Gate gate;gate.Publish({true,1,2,1000});assert(gate.Arm(1,2,1000));
    assert(gate.Evaluate(2,2,1,2,1001,true)==centurionnative::Decision::Block); // immediate store revokes armed token
    gate.Publish({true,1,2,1010,false});assert(!gate.Arm(1,2,1010));
    assert(gate.Evaluate(2,2,1,2,1010)==centurionnative::Decision::Block);
    assert(gate.Evaluate(2,3,1,2,1010)==centurionnative::Decision::Forward); // other/NPC untouched
    gate.Publish({true,1,2,1020,true});assert(gate.Arm(1,2,1020));
    assert(gate.Evaluate(2,2,1,2,1020)==centurionnative::Decision::Permit);
    using P=centurionreload::Phase;using E=centurionreload::Event;
    centurionreload::Config cfg;cfg.cylinderGestures=cfg.circleOpens=cfg.upwardEjection=true;
    centurionreload::Frame f;f.enabled=f.context=f.supported=f.tracked=f.ammoFresh=true;
    f.pawn=1;f.weapon=2;f.clip=3;f.reserve=18;f.capacity=6;f.nowMs=1000;
    centurionreload::State s;centurionreload::Update(s,f,cfg);f.releaseCylinder=true;
    assert(centurionreload::Update(s,f,cfg).event==E::Opened);f.releaseCylinder=false;
    f.gun.q=centurionclip::Degrees(-90,0,0);f.nowMs+=100;
    assert(centurionreload::Update(s,f,cfg).event==E::None);f.nowMs+=150;
    assert(centurionreload::Update(s,f,cfg).event==E::None&&s.phase==P::Open);
    f.gun.q=centurionclip::Degrees(90,0,0);++f.nowMs;
    assert(centurionreload::Update(s,f,cfg).event==E::None);f.nowMs+=100;
    assert(centurionreload::Update(s,f,cfg).event==E::Dumped);
    centurionshells::WorldFrame world{{},{},0,0,52.5f};centurionshells::Burst burst;
    assert(centurionshells::Launch(burst,f,E::Dumped,true,world,true));
    const auto muzzle=hipknife::Rotate(f.gun.q,{0,0,-1});
    for(unsigned i=0;i<burst.count;++i){
        const auto trackingVelocity=centurionshells::ToTracking(burst.cases[i].velocity,world);
        assert(hipknife::Dot(trackingVelocity,muzzle)<-.3f); // rearward, NEVER out of muzzle
    }
    cfg.deliberateClose=true;s.phase=P::Loaded;s.since=f.nowMs;s.stageGun=f.gun;
    s.startClip=f.clip;s.startReserve=f.reserve;s.startCapacity=f.capacity;
    // Ordinary slow placement/stowing movement must NOT authorize reload.
    for(unsigned step=0;step<12;++step){f.nowMs+=100;f.gun.p.x+=.02f;
        assert(!centurionreload::Update(s,f,cfg).requestNative&&s.phase==P::Loaded);}
    f.nowMs+=20;f.gun.p.x+=.09f;assert(centurionreload::Update(s,f,cfg).requestNative);
    GripWitness witness{{1,2,3,1000,{0,0,0},true},Identity()};
    assert(witness.Current(1,2,3,1100)&&!witness.Current(1,2,3,1101)&&!witness.Current(1,2,4,1000));
    f.context=false;assert(centurionreload::Update(s,f,cfg).event==E::Cancelled);
    printf("PASS physical polish: %u moving-chain held-right/palm/size cases; holster token refusal, NPC forwarding, UP-only dump, rearward cases, stale grip refusal. NOT Steam/PSVR2\n",cases);
}
