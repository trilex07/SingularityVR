#include "../deadlock_gesture.h"
#include <cassert>
#include <limits>

using namespace deadlockgesture;

Frame Sample(std::uint64_t time, bool grip, V hand={.22f,-.13f,.12f}) {
    return {true,grip,{},hand,0,time};
}

void Arm(Gesture& g, std::uint64_t t) {
    assert(g.Update(Sample(t,false))==Event::None);
    assert(g.Update(Sample(t+10,true))==Event::Armed);
}

int main() {
    Gesture g;
    assert(g.Update(Sample(10,true))==Event::None); // held on entry
    assert(g.Update(Sample(20,false))==Event::None);
    assert(g.Update(Sample(30,true,{-.22f,-.13f,.12f}))==Event::None); // ammo shoulder
    assert(g.Update(Sample(40,true))==Event::None); // cannot acquire mid-squeeze
    Arm(g,50);
    assert(g.Update(Sample(70,true,{.20f,-.13f,.06f}))==Event::None);
    assert(g.Update(Sample(95,true,{.13f,-.14f,-.04f}))==Event::None);
    assert(g.Update(Sample(104,false,{.09f,-.18f,-.20f}))==Event::ThrowIntent);
    assert(g.Update(Sample(114,false))==Event::None);
    Arm(g,130);
    auto headOnly=Sample(165,false);
    headOnly.head={.8f,0,.6f};headOnly.yaw=2.f;
    assert(g.Update(headOnly)==Event::Cancelled);
    Arm(g,180);
    auto busy=Sample(205,true);busy.eligible=false;
    assert(g.Update(busy)==Event::Cancelled);
    assert(g.Update(Sample(215,true))==Event::None);
    assert(g.Update(Sample(240,false,{.1f,0,-.3f}))==Event::None);
    Arm(g,250);
    assert(g.Update(Sample(400,false,{.1f,0,-.3f}))==Event::Cancelled);
    Arm(g,450);
    auto lost=Sample(480,false,{.1f,0,-.3f});lost.eligible=false;
    assert(g.Update(lost)==Event::Cancelled);
    Arm(g,500);
    assert(g.Update(Sample(545,false,{.3f,0,.46f}))==Event::Cancelled);
    Arm(g,560);
    auto invalid=Sample(590,false);invalid.hand.x=std::numeric_limits<float>::quiet_NaN();
    assert(g.Update(invalid)==Event::Cancelled);
    assert(g.ArmPulse(591)==0.f);

    // Mirrored: TMD in right hand, left shoulder arms; right shoulder is for ammo.
    const auto mirror = [](std::uint64_t t, bool grip, V hand={-.22f,-.13f,.12f}) {
        return Frame{true,grip,{},hand,0,t,true};
    };
    Gesture m;
    assert(m.Update(mirror(1000,false))==Event::None);
    assert(m.Update(mirror(1010,true,{.22f,-.13f,.12f}))==Event::None);
    assert(m.ArmPulse(1010)==0.f);
    assert(m.Update(mirror(1020,true))==Event::None);
    assert(m.Update(mirror(1030,false))==Event::None);
    assert(m.Update(mirror(1040,true))==Event::Armed);
    assert(m.ArmPulse(1039)==0.f);
    assert(m.ArmPulse(1040)==.22f);
    assert(m.ArmPulse(1084)==.22f);
    assert(m.ArmPulse(1085)==0.f);
    assert(m.Update(mirror(1075,true,{-.16f,-.14f,-.02f}))==Event::None);
    assert(m.Update(mirror(1080,false,{-.10f,-.14f,-.24f}))==Event::ThrowIntent);
    assert(m.ArmPulse(1080)==0.f);
}
