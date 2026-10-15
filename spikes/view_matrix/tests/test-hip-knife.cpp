#include "../hip_knife.h"
#include <cassert>
#include <cstdio>
#include <limits>
#include <chrono>
using namespace hipknife;
static unsigned checks=0;
#define CHECK(x) do{++checks;assert(x);}while(0)
static Frame Base(){
    Frame f;f.enabled=f.gameplay=f.focused=f.alive=f.nativeKnife=true;f.pawn=123;
    f.head={0,1,0};f.hip=Hip(f.head,0);f.nowMs=100;
    for(int h=0;h<2;++h){f.free[h]=f.tracked[h]=true;f.hand[h].p=f.hip.p;}
    return f;
}
static void Pick(State& s,Frame& f,int h=0){
    f.grip[0]=f.grip[1]=false;CHECK(Update(s,f)==Event::None);
    f.nowMs+=11;f.grip[h]=true;CHECK(Update(s,f)==Event::Picked);CHECK(s.held==h);
}
static void Move(State& s,Frame& f,V p,unsigned ms=11){f.nowMs+=ms;f.hand[s.held].p=p;Update(s,f);}
static void Away(State& s,Frame& f){
    Move(s,f,{-0.2f,1,-.4f},400); // time gap intentionally cannot trigger
    f.nowMs+=11;CHECK(Update(s,f)==Event::None);CHECK(s.armed);
}
int main(){
    {const Pose hip=Hip({0,1,0},0);CHECK(hip.p.x<0&&hip.p.y==.5f&&hip.p.z<0);
     const Pose turned=Hip({0,1,0},3.14159265f*.5f);CHECK(std::fabs(turned.p.x+.06f)<1e-5f&&turned.p.z>.24f);
     const Q down=Multiply(hip.q,{-.70710678f,0,0,.70710678f});CHECK(Rotate(down,{0,0,-1}).y<-.999f);}
    {State s;Frame f=Base();f.grip[0]=true;CHECK(Update(s,f)==Event::None);CHECK(s.held==-1);
     Pick(s,f);CHECK(Reserves(s,0)&&!Reserves(s,1));CHECK(!s.Pulse(f.nowMs));
     f.grip[0]=false;CHECK(Update(s,f)==Event::Sheathed);CHECK(s.held==-1);}
    {State s;Frame f=Base();f.free[1]=false;f.grip[0]=f.grip[1]=false;Update(s,f);
     f.grip[1]=true;CHECK(Update(s,f)==Event::None);CHECK(s.held==-1);}
    {State s;Frame f=Base();f.hand[0].p.x=-1;f.grip[0]=false;Update(s,f);
     f.grip[0]=true;CHECK(Update(s,f)==Event::None);CHECK(s.held==-1);}
    {State s;Frame f=Base();Pick(s,f,1);Away(s,f);
     const V p=f.hand[1].p;
     for(int i=1;i<=30;++i)Move(s,f,p+V{0,0,-.001f*float(i)});
     CHECK(!s.Pulse(f.nowMs)); // slow movement never attacks
    }
    {State s;Frame f=Base();Pick(s,f);Away(s,f);const V p=f.hand[0].p;
     unsigned swings=0;
     for(int i=1;i<=5;++i){f.nowMs+=11;f.hand[0].p=p+V{.025f*float(i),0,0};
        const Event e=Update(s,f);if(e==Event::Swing)++swings;else CHECK(e==Event::None);}
     CHECK(swings==1);
     CHECK(s.Pulse(f.nowMs));CHECK(!s.Pulse(f.nowMs+90));
     const auto first=s.lastSwingMs;
     for(int i=0;i<20;++i)Move(s,f,f.hand[0].p+V{.025f,0,0});
     CHECK(s.lastSwingMs==first); // sustained fast motion is one attack
     f.nowMs+=11;Update(s,f);CHECK(s.armed);
     f.nowMs+=750;Update(s,f);CHECK(!s.armed); // gap invalidates rearm and pulse
    }
    for(int gate=0;gate<9;++gate){State s;Frame f=Base();Pick(s,f);Away(s,f);
        switch(gate){case 0:f.enabled=false;break;case 1:f.gameplay=false;break;
        case 2:f.focused=false;break;case 3:f.alive=false;break;case 4:f.nativeKnife=false;break;
        case 5:f.pawn=999;break;case 6:f.tracked[0]=false;break;case 7:f.free[0]=false;break;
        default:f.hand[0].q={0,0,0,0};}
        CHECK(Update(s,f)==Event::Reset);CHECK(s.held==-1&&!s.Pulse(f.nowMs));
        f=Base();f.grip[0]=true;Update(s,f);CHECK(s.held==-1);
    }
    {State s;Frame f=Base();Pick(s,f);Away(s,f);Move(s,f,{2,1,-.4f});CHECK(!s.armed&&!s.Pulse(f.nowMs));}
    {State s;Frame f=Base();Pick(s,f);Away(s,f);
     unsigned swings=0;
     for(int i=1;i<=8;++i){const float a=float(i)*.13f;f.hand[0].q={0,std::sin(a*.5f),0,std::cos(a*.5f)};
        f.nowMs+=11;if(Update(s,f)==Event::Swing)++swings;}
     CHECK(swings==1); // rotation at the wrist, not just translation
     f.nowMs+=11;Update(s,f);CHECK(s.armed);
     const auto prior=s.lastSwingMs;
     for(int i=0;i<5;++i)Move(s,f,f.hand[0].p+V{.03f,0,0});
     CHECK(s.lastSwingMs==prior); // slow rearm cannot bypass cooldown
    }
    {State s;Frame f=Base();Pick(s,f);Move(s,f,f.hip.p,11);
     unsigned swings=0;
     for(int i=0;i<8;++i){f.hand[0].p.x+=.025f;f.nowMs+=11;if(Update(s,f)==Event::Swing)++swings;}
     CHECK(swings==0); // grace during drawing (and still near hip)
    }
    {State s;Frame f=Base();Pick(s,f);f.hand[0].p.x=std::numeric_limits<float>::quiet_NaN();CHECK(Update(s,f)==Event::Reset);}
    {State s;Frame f=Base();f.grip[0]=f.grip[1]=false;Update(s,f);f.grip[0]=f.grip[1]=true;
     CHECK(Update(s,f)==Event::Picked);CHECK(s.held==0);}
    {const Pose pose{{1,2,3},{0,std::sin(.4f),0,std::cos(.4f)}};const V v{.2f,-.1f,-.4f};
     CHECK(Length(EyeLocal(pose.p+Rotate(pose.q,v),pose)-v)<1e-5f);
     float m[4][4];CHECK(Projection(1,1,m));CHECK(m[2][3]==-1);CHECK(!Projection(0,1,m));}
    {TriggerGate g;CHECK(g.Update(false,.7f)==.7f);CHECK(g.Update(true,.7f)==0);
     CHECK(g.Update(false,.7f)==0);CHECK(g.Update(false,0)==0);CHECK(g.Update(false,.7f)==.7f);}
    const auto start=std::chrono::steady_clock::now();
    State s;Frame f=Base();Pick(s,f);Away(s,f);
    for(unsigned i=0;i<360000;++i){f.nowMs+=11;Update(s,f);}
    const auto ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
    std::printf("PASS hip knife: %u checks + 360000 CPU frames %.3f ms; no GPU/Steam\n",checks,ms);
}
