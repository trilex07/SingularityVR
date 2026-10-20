#include "../body_equipment_policy.h"
#include <cassert>
#include <limits>
#include <cstdio>
using namespace bodyequipment;
static Frame frame(){
    Frame f;f.enabled=f.gameplay=f.focused=f.alive=true;f.pawn=1;f.grenade=2;f.heavy=3;f.now=100;
    f.tracked[0]=f.tracked[1]=f.free[0]=f.free[1]=true;return f;
}
static void ack(Frame& f,const Request& r){f.ack={r.sequence,r.pawn,r.item,r.command,true};f.now+=10;}
int main(){
    Layout layout;
    for(int degrees=0;degrees<360;++degrees){
        const float yaw=degrees*.01745329252f;const V head{.2f,1.1f,-.5f};
        const auto g=layout.At(Slot::Grenade,head,yaw),h=layout.At(Slot::Heavy,head,yaw);
        const hipknife::Q inv{0,-std::sin(yaw*.5f),0,std::cos(yaw*.5f)};
        const auto local=hipknife::Rotate(inv,g.centre-head);
        assert(hipknife::Length(local-layout.grenade)<.00001f);
        assert(hipknife::Length(g.centre-h.centre)>g.radius+h.radius);
        const Zone zones[]{g,h};assert(Pick(g.centre,zones,2)==Slot::Grenade);assert(Pick(h.centre,zones,2)==Slot::Heavy);
    }
    assert(layout.At(Slot::Grenade,{},0).centre.x>0);
    assert(layout.At(Slot::Grenade,{},0).centre.z>0);
    assert(layout.At(Slot::Heavy,{},0).centre.y<layout.grenade.y);
    assert(layout.At(Slot::None,{},0).slot==Slot::None);
    const Zone overlap[]{{Slot::Grenade,{.1f,0,0},.18f},{Slot::Heavy,{.08f,0,0},.16f},{Slot::Gun,{.06f,0,0},.16f}};
    assert(Pick({},overlap,3)==Slot::None);assert(Pick({},nullptr,1)==Slot::None);
    const float nan=std::numeric_limits<float>::quiet_NaN();assert(Pick({nan,0,0},overlap,3)==Slot::None);
    State s;auto f=frame();Update(s,f);f.grip[1]=true;f.rightSlot=Slot::Grenade;
    auto draw=Update(s,f);assert(draw.command==Command::DrawGrenade&&s.phase==Phase::DrawGrenade);
    assert(Update(s,f).command==Command::None);
    auto stale=draw;stale.pawn=4;ack(f,stale);Update(s,f);assert(s.phase==Phase::DrawGrenade);
    ack(f,draw);Update(s,f);assert(s.phase==Phase::Safe);
    f.pinVerified=true;f.pin=f.hand[0].p={0,0,-.1f};f.grip[0]=true;Update(s,f);assert(s.phase==Phase::PinPull);
    // Rigidly translating and rotating both hands does not extract a pin.
    const hipknife::Q turn{0,.70710678f,0,.70710678f};
    f.hand[1]={{.2f,0,.3f},turn};f.hand[0].p=f.hand[1].p+hipknife::Rotate(turn,{0,0,-.1f});
    Update(s,f);assert(s.phase==Phase::PinPull);
    f.hand[0].p=f.hand[1].p+hipknife::Rotate(turn,{0,0,-.21f});
    auto pin=Update(s,f);assert(pin.command==Command::PullPin&&s.phase==Phase::ArmPending);
    assert(Update(s,f).command==Command::None);
    ack(f,pin);Update(s,f);assert(s.phase==Phase::Armed);
    f.now+=20;f.hand[1].p.x+=.04f;Update(s,f);
    f.now+=20;f.hand[1].p.x+=.04f;Update(s,f);
    f.now+=10;f.grip[1]=false;auto toss=Update(s,f);
    assert(toss.command==Command::ThrowGrenade&&toss.velocity.x>.65f&&toss.velocity.x<12);
    assert(Update(s,f).command==Command::None);ack(f,toss);Update(s,f);assert(s.phase==Phase::Idle);
    s.Clear();f=frame();Update(s,f);f.grenade=0;f.grip[1]=true;f.rightSlot=Slot::Grenade;
    assert(Update(s,f).command==Command::None);
    f.grip[1]=false;Update(s,f);f.rightSlot=Slot::Heavy;f.grip[1]=true;
    auto heavy=Update(s,f);assert(heavy.command==Command::DrawHeavy);ack(f,heavy);Update(s,f);assert(s.phase==Phase::Heavy);
    assert(Update(s,f).command==Command::None);f.grip[1]=false;Update(s,f);f.grip[1]=true;
    auto stow=Update(s,f);assert(stow.command==Command::StowHeavy);ack(f,stow);Update(s,f);assert(s.phase==Phase::Idle);
    // Context/identity loss and timeout never synthesize a throw or fuse write.
    for(unsigned reason=0;reason<5;++reason){
        s.Clear();f=frame();Update(s,f);f.rightSlot=Slot::Grenade;f.grip[1]=true;draw=Update(s,f);
        if(reason==0)f.focused=false;else if(reason==1)f.pawn=4;
        else if(reason==2)f.tracked[1]=false;else if(reason==3)f.grenade=4;
        else f.now+=1001;
        assert(Update(s,f).command==Command::None&&s.phase==Phase::Idle);
    }
    std::puts("PASS body equipment: yaw anchors, exclusive slots, native ACKs, relative pin pull, throw edge and lifecycle refusal [CPU ONLY]");
}
