#include "../sniper_scope.h"
#include "../hand_anchor_math.h"
#include "../weapon_muzzle_geometry.h"
#include <cassert>
#include <cstdio>
int main(){
    using namespace sniperscope;
    Mount m;Pose hand{{0,0,0},{}};
    assert(Capture(m,1,2,hand,{0,0,-.12f},{0,0,-.22f}));
    Pose eyes[2]{{{-.032f,0,0},{}},{{.032f,0,0},{}}};State s;
    assert(!s.Update(true,m,1,2,hand,eyes,100));
    assert(!s.Update(true,m,1,2,hand,eyes,199));
    assert(s.Update(true,m,1,2,hand,eyes,200));
    hand.p.x=.025f;assert(s.Update(true,m,1,2,hand,eyes,210));
    hand.p.x=.3f;assert(!s.Update(true,m,1,2,hand,eyes,220));
    hand.p.x=0;assert(!s.Update(true,m,1,3,hand,eyes,230));
    assert(!s.Update(false,m,1,2,hand,eyes,240));
    hand.q={0,1,0,0};assert(!s.Update(true,m,1,2,hand,eyes,250));
    hand.q={};hand.p.z=.5f;assert(!s.Update(true,m,1,2,hand,eyes,260));
    Mount invalid;assert(!Capture(invalid,1,2,hand,{0,0,0},{0,0,0}));
    assert(!Capture(invalid,0,2,hand,{0,0,0},{0,0,-.1f}));
    assert(!Capture(invalid,1,2,hand,{NAN,0,0},{0,0,-.1f}));
    // No timeout tied to mesh visibility: the native fullscreen scope hides it.
    hand={};s={};assert(!s.Update(true,m,1,2,hand,eyes,100));
    assert(s.Update(true,m,1,2,hand,eyes,200));assert(s.Update(true,m,1,2,hand,eyes,10000));
    // An aim animation moves the rendered lens before ScopeActive sees an
    // overlay. That must not cancel the proximity-generated native aim input.
    s={};assert(!s.Update(true,m,1,2,hand,eyes,100));
    auto animated=m;animated.lens.z-=.5f;
    assert(s.Update(true,animated,1,2,hand,eyes,200));
    assert(s.Update(true,animated,1,2,hand,eyes,250));
    assert(s.eyeIndex>=0&&s.axial>.11f&&s.axial<.13f);
    hand.p.x=.3f;assert(!s.Update(true,animated,1,2,hand,eyes,260));
    hand={};assert(!s.Update(false,m,1,2,hand,eyes,270));
    assert(!s.Update(true,m,1,2,hand,eyes,280));
    assert(s.Update(true,m,1,2,hand,eyes,380));
    for(unsigned i=0;i<360;++i){
        const float yaw=float(i)*.01745329252f;
        const hipknife::Q rotation{0,std::sin(yaw*.5f),0,std::cos(yaw*.5f)};
        const V shift{1.f,.4f,-2.f};
        Pose movedHand{shift,rotation},movedEyes[2];
        for(unsigned e=0;e<2;++e)movedEyes[e]={shift+hipknife::Rotate(rotation,eyes[e].p),rotation};
        State rotated;
        assert(!rotated.Update(true,m,1,2,movedHand,movedEyes,100));
        assert(rotated.Update(true,m,1,2,movedHand,movedEyes,200));
        // A new weapon identity must dwell again, not inherit zoom/hysteresis.
        auto newMount=m;newMount.weapon=3;
        assert(!rotated.Update(true,newMount,1,3,movedHand,movedEyes,210));
    }
    // Round-trip the exact XR/world mapping for recenter yaw/scale/seated height.
    for(int i=0;i<2000;++i){
        const float yaw=float(i%41)*.13f,base=float(i%31)*.11f,units=52.5f;
        const hipknife::V centre{1,.5f,-3},p{.1f,float(i%17)*.05f,-.3f};
        const auto v=vrpose::TrackingDeltaToWorldUU({p.x-centre.x,p.y-centre.y,p.z-centre.z},yaw,base,units);
        hipknife::V actual;assert(weaponmuzzle::ToTracking({v.x,v.y,v.z},centre,yaw,base,units,actual));
        assert(hipknife::Length(actual-p)<.00001f);
    }
    for(int enabled=0;enabled<2;++enabled)for(int gloves=0;gloves<2;++gloves)
    for(int live=0;live<2;++live)for(int holstered=0;holstered<2;++holstered)
    for(int support=0;support<2;++support)for(long verts:{5799L,8297L,11387L,11389L})
        assert(handanchor::KasimovSingleGrip(enabled!=0,gloves!=0,live!=0,holstered!=0,verts,support!=0)==
            bool(enabled&&gloves&&live&&!holstered&&verts==11389&&!support));
    puts("PASS near-eye scope identity, relief, alignment, hysteresis, hidden mesh, 2000 XR roundtrips, sniper-only grip gates");
}
