#include "../weapon_steady.h"
#include "../hand_hud_policy.h"
#include <cassert>
#include <cstdio>
static void SupportedTremor(unsigned hz,float frequency){
    weaponsteady::Filter hands[2];
    double rawSq=0,filteredSq=0;
    const unsigned period=1000/hz;
    for(unsigned i=0;i<10000/period;++i){
        const float t=float(i*period)*.001f;
        hipknife::Pose raw[2]{};raw[0].p.z=-.35f;raw[1].p.z=-.60f;
        // Opposed 3mm hand tremor is a conservative two-hand angular case.
        raw[0].p.x=.003f*std::sin(6.2831853f*frequency*t);
        raw[1].p.x=-raw[0].p.x;
        const float a=.008726646f*std::sin(6.2831853f*frequency*t);
        raw[0].q=raw[1].q={0,std::sin(a),0,std::cos(a)};
        const auto left=hands[0].Update(raw[0],true,true,1,2,1000+i*period);
        const auto right=hands[1].Update(raw[1],true,true,1,2,1000+i*period);
        assert(hipknife::Valid(left.q)&&hipknife::Valid(right.q));
        if(i*period>1000){
            const float before=std::atan2(raw[1].p.x-raw[0].p.x,.25f);
            const float after=std::atan2(right.p.x-left.p.x,.25f);
            rawSq+=before*before;filteredSq+=after*after;
            assert(std::fabs(left.q.y)<.002f);
        }
    }
    const double ratio=std::sqrt(filteredSq/rawSq);
    std::printf("supported tremor %uHz samples / %.0fHz motion: RMS %.3f of raw\n",hz,frequency,ratio);
    assert(ratio<.22); // >=78% reduction in supported angular RMS, 5--12Hz.
}
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
    for(unsigned hz: {50u,90u,120u})for(float frequency: {5.f,8.f,12.f})
        SupportedTremor(hz,frequency);
    // A continuous intentional turn must not stick; release immediately
    // re-seeds the unfiltered pose, rather than retaining strong-filter lag.
    Filter turn;float maximumLag=0;
    for(unsigned i=0;i<=300;++i){
        const float angle=float(i)*.011f*.174532925f; // 10 degrees/second.
        raw={};raw.q={0,std::sin(angle*.5f),0,std::cos(angle*.5f)};
        const auto filtered=turn.Update(raw,true,true,1,2,1000+11*i);
        maximumLag=std::max(maximumLag,angle-2.f*std::atan2(filtered.q.y,filtered.q.w));
    }
    std::printf("intentional 10deg/s turn: maximum lag %.3fdeg\n",maximumLag*57.29578f);
    assert(maximumLag<.035f); // <2deg, and hard/fast turns still snap above.
    auto released=turn.Update(raw,true,false,1,2,4311);
    assert(released.q.y==raw.q.y&&released.q.w==raw.q.w);
    Filter clutch;raw={};clutch.Update(raw,true,true,1,2,1000);
    for(unsigned i=1;i<=80;++i){
        raw.p.x=.011f*std::sin(float(i));
        raw.q={0,std::sin(.01f),0,std::cos(.01f)};
        auto held=clutch.Update(raw,true,true,1,2,1000+11*i);
        assert(held.p.x==0.f&&held.q.y==0.f); // exact hold, no accumulated creep
    }
    raw.p.x=.013f;
    assert(clutch.Update(raw,true,true,1,2,1900).p.x==raw.p.x);
    raw.q={0,std::sin(.03f),0,std::cos(.03f)};
    assert(clutch.Update(raw,true,true,1,2,1911).q.y==raw.q.y);
    raw.p.x=.014f;
    assert(clutch.Update(raw,true,false,1,2,1922).p.x==raw.p.x);
    std::puts("PASS weapon steady: jitter attenuation, movement, identity/gap/release reset, quaternion sign and HUD gates");
}
