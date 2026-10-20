#include "../weapon_steady.h"
#include "../hand_hud_policy.h"
#include <cassert>
#include <cstdio>
int main(){
    using namespace weaponsteady;
    assert(Hold(true,true,true,true,true,true,false));
    for(int missing=0;missing<6;++missing){bool b[6]={true,true,true,true,true,true};b[missing]=false;
        assert(!Hold(b[0],b[1],b[2],b[3],b[4],b[5],false));}
    assert(!Hold(true,true,true,true,true,true,true));
    assert(handhud::HideHealth(true)&&!handhud::HideHealth(false));
    assert(handhud::HideAmmo(true)&&!handhud::HideAmmo(false));
    Filter mild,strong;hipknife::Pose raw{};
    mild.Update(raw,true,false,1,2,1000);strong.Update(raw,true,true,1,2,1000);
    float inputSq=0,mildSq=0,strongSq=0;
    for(unsigned i=1;i<=180;++i){raw.p.x=(i&1)?.001f:-.001f;
        const auto a=mild.Update(raw,true,false,1,2,1000+11*i);
        const auto b=strong.Update(raw,true,true,1,2,1000+11*i);
        inputSq+=raw.p.x*raw.p.x;mildSq+=a.p.x*a.p.x;strongSq+=b.p.x*b.p.x;
        assert(hipknife::Valid(a.q)&&hipknife::Valid(b.q));
    }
    assert(mildSq<inputSq*.25f&&strongSq<mildSq*.4f);
    raw.p.x=.2f;assert(strong.Update(raw,true,true,1,2,3000).p.x==.2f);
    raw.p.x=.201f;assert(strong.Update(raw,false,true,1,2,3011).p.x==raw.p.x);
    raw.p.x=.202f;assert(strong.Update(raw,true,true,1,3,3022).p.x==raw.p.x);
    raw.q={0,0,0,-1};auto q=strong.Update(raw,true,true,1,3,3033).q;
    assert(hipknife::Valid(q));
    raw.q={0,0,.5f,.8660254f};q=strong.Update(raw,true,true,1,3,3044).q;
    assert(q.z==raw.q.z);
    raw.p.x=.203f;assert(strong.Update(raw,true,true,1,3,4000).p.x==raw.p.x);
    raw.p.x=.204f;assert(strong.Update(raw,true,false,1,3,4011).p.x==raw.p.x);
    Filter slow,fast;hipknife::Pose zero{},step{};step.p.x=.001f;
    slow.Update(zero,true,true,1,2,1000);fast.Update(zero,true,true,1,2,1000);
    for(unsigned i=1;i<=10;++i)slow.Update(step,true,true,1,2,1000+20*i);
    for(unsigned i=1;i<=20;++i)fast.Update(step,true,true,1,2,1000+10*i);
    assert(std::fabs(slow.value.p.x-fast.value.p.x)<.000002f);
    Filter rotational;rotational.Update(zero,true,true,1,2,1000);
    for(unsigned i=1;i<=180;++i){const float a=(i&1)?.001f:-.001f;
        raw={};raw.q={0,std::sin(a),0,std::cos(a)};
        q=rotational.Update(raw,true,true,1,2,1000+11*i).q;
        assert(std::fabs(q.y)<.0002f&&hipknife::Valid(q));}
    std::puts("PASS weapon steady: jitter attenuation, movement, identity/gap/release reset, quaternion sign and HUD gates");
}
