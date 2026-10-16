#include "../centurion_shells.h"
#include <cassert>
#include <cstdio>
#include <limits>
#include <chrono>
using namespace centurionshells;
static bool close(V a,V b,float tolerance=.0001f){return hipknife::Length(a-b)<tolerance;}
int main(){
    for(unsigned i=0;i<100000;++i)assert(!NeedsPass(true,false,false,0));
    assert(NeedsPass(false,false,false,0)&&NeedsPass(true,true,false,0)&&
           NeedsPass(true,false,true,0)&&NeedsPass(true,false,false,6));
    centurionreload::Frame f;f.enabled=f.context=f.tracked=f.supported=f.ammoFresh=true;
    f.pawn=7;f.weapon=13;f.clip=0;f.reserve=20;f.capacity=6;f.nowMs=1000;
    f.socket={.25f,1.f,-.45f};f.gun={f.socket,centurionclip::Degrees(-45,0,0)};
    WorldFrame world{{100,-200,5},{.5f,1.2f,-.3f},.7f,-1.1f,80};
    Burst b;Pose out[6];
    using E=centurionreload::Event;
    assert(!Launch(b,f,E::None,true,world));assert(!Launch(b,f,E::Dumped,false,world));
    for(int bad=0;bad<9;++bad){auto invalid=f;
        if(bad==0)invalid.enabled=false;if(bad==1)invalid.context=false;
        if(bad==2)invalid.tracked=false;if(bad==3)invalid.supported=false;
        if(bad==4)invalid.ammoFresh=false;if(bad==5)invalid.pawn=0;
        if(bad==6)invalid.weapon=0;if(bad==7)invalid.clip=-1;if(bad==8)invalid.capacity=7;
        assert(!Launch(b,invalid,E::Dumped,true,world));
    }
    assert(Launch(b,f,E::Dumped,true,world));
    assert(!Launch(b,f,E::Dumped,true,world)); // once, not per eye/draw/button
    assert(b.count==6&&f.clip==0&&f.reserve==20);
    assert(Sample(b,world,7,13,1000,out)==6);
    for(const auto& p:out)assert(hipknife::Length(p.p-f.socket)<.0051f);
    assert(!Sample(b,world,7,14,1000,out)&&!Sample(b,world,7,13,999,out));
    assert(!Sample(b,world,7,13,1650,out));
    assert(Sample(b,world,7,13,1300,out)==6);
    Pose saved[6];for(unsigned i=0;i<6;++i)saved[i]=out[i];
    // No hand pose input to Sample: a moving hand can't drag an existing case.
    f.gun.p={5,9,-8};f.socket=f.gun.p;
    assert(Sample(b,world,7,13,1300,out)==6);
    for(unsigned i=0;i<6;++i)assert(close(out[i].p,saved[i].p));
    // Each independent sample is exact in absolute time; 20/72/90/144 FPS do
    // not alter trajectory. Simulate moving camera/recenter and snap turn.
    for(unsigned step=1;step<650;++step){
        const float t=step*.001f;
        WorldFrame moving=world;moving.origin=moving.origin+V{40*t,-5*t,0};
        moving.baseYaw+=t*1.7f;moving.centreYaw-=t*.4f;
        moving.centre=moving.centre+V{.1f,-.2f,.3f};
        assert(Sample(b,moving,7,13,1000+step,out)==6);
        for(unsigned i=0;i<6;++i){
            const auto expected=b.cases[i].origin+b.cases[i].velocity*t+V{0,0,-4.905f*80*t*t};
            assert(close(ToWorld(out[i].p,moving),expected,.0002f));
        }
    }
    // Exact inversion of production TrackingDeltaToWorldUU, independently of
    // world yaw, recenter yaw, origin and calibrated scale. Orientation follows
    // the same world direction after a snap, not merely position.
    unsigned frames=0;
    for(int y=-6;y<=6;++y)for(int c=-6;c<=6;++c)for(float units:{10.f,52.5f,80.f,200.f}){
        WorldFrame w=world;w.baseYaw=y*.51f;w.centreYaw=c*.47f;w.units=units;
        for(V p:{V{0,0,0},V{.3f,1.f,-.6f},V{-3.f,2.f,4.f}})
            assert(close(ToTracking(ToWorld(p,w),w),p,.0001f));
        w.units=80;assert(Sample(b,w,7,13,1000,out)==6);
        const auto bornForward=WorldDelta(hipknife::Rotate(b.cases[0].orientation,{0,0,-1}),world);
        const auto currentForward=WorldDelta(hipknife::Rotate(out[0].q,{0,0,-1}),w);
        assert(close(bornForward,currentForward,.0002f));++frames;
    }
    for(int bad=0;bad<10;++bad){auto copy=b;auto invalid=f;
        if(bad==0)invalid.enabled=false;if(bad==1)invalid.context=false;
        if(bad==2)invalid.tracked=false;if(bad==3)invalid.supported=false;
        if(bad==4)invalid.ammoFresh=false;if(bad==5)invalid.pawn=8;
        if(bad==6)invalid.weapon=14;if(bad==7)invalid.nowMs=999;if(bad==8)invalid.nowMs=1650;
        Maintain(copy,invalid,bad==9?E::Cancelled:E::None);assert(!copy.count);
    }
    f.clip=2;f.nowMs=1700;assert(Launch(b,f,E::Dumped,true,world)&&b.count==4);
    f.clip=6;f.nowMs=1800;assert(!Launch(b,f,E::Dumped,true,world));
    auto invalid=world;invalid.units=std::numeric_limits<float>::quiet_NaN();
    assert(!Sample(b,invalid,7,13,1800,out));
    invalid=world;invalid.units=81;assert(!Sample(b,invalid,7,13,1800,out));
    f.clip=0;f.nowMs=2000;assert(Launch(b,f,E::Dumped,true,world));
    const auto start=std::chrono::steady_clock::now();
    for(unsigned i=0;i<100000;++i)assert(Sample(b,world,7,13,2000+i%650,out)==6);
    const double microseconds=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count()/100000.;
    std::printf("six-case CPU Sample mean %.3f us (synthetic only, NOT game frame-time proof)\n",microseconds);
    b.count=7;assert(!Sample(b,world,7,13,2100,out));
    std::printf("L1 shells PASS: bounded original-mesh flight, once/event, lifecycle, 649 times, %u reference frames, conserved input counts\n",frames);
}
