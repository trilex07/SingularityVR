#include "../weapon_accuracy_policy.h"
#include "../two_hand_spread.h"
#include "../weapon_trace_math.h"
#include <cassert>
#include <cmath>
#include <cstdio>

static float DirectionAngle(int pitch,int yaw){
    const float unit=6.28318530718f/65536.f;
    const float p=pitch*unit,y=yaw*unit;
    return std::acos(std::cos(p)*std::cos(y))*180.f/3.14159265359f;
}
int main(){
    using weaponreload::Kind;
    using weaponaccuracy::EffectivePercent;
    assert(EffectivePercent(11387,Kind::Valkyrie,true,false,50,70,85)==70);
    assert(EffectivePercent(11387,Kind::Valkyrie,true,true,50,70,85)==35);
    assert(EffectivePercent(8297,Kind::Volk,true,true,50,70,85)==43);
    assert(EffectivePercent(11387,Kind::Volk,true,true,50,70,85)==100);
    assert(EffectivePercent(8297,Kind::Volk,false,true,50,70,85)==100);
    assert(EffectivePercent(5799,Kind::None,true,true,50,70,85)==100);
    assert(EffectivePercent(11387,Kind::Valkyrie,true,true,50,-2,85)==100);
    unsigned seed=0x12345678;double nativeSquare=0,rifleSquare=0,
        rifleSupportedSquare=0,volkSquare=0,volkSupportedSquare=0;
    for(int i=0;i<10000;++i){
        seed=1664525u*seed+1013904223u;const int pitch=int(seed%801)-400;
        seed=1664525u*seed+1013904223u;const int yaw=int(seed%801)-400;
        auto sample=[&](int percent){int v[3]{pitch,yaw,271};
            twohandspread::Apply(v,0,0,percent,true,true);
            assert(v[2]==271);return double(DirectionAngle(v[0],v[1]));};
        const double n=sample(100),r=sample(70),rs=sample(35),
            v=sample(85),vs=sample(43);
        nativeSquare+=n*n;rifleSquare+=r*r;rifleSupportedSquare+=rs*rs;
        volkSquare+=v*v;volkSupportedSquare+=vs*vs;
    }
    auto rms=[](double x){return std::sqrt(x/10000);};
    std::printf("angular RMS degrees native=%.4f Valkyrie=%.4f Valkyrie-2h=%.4f Volk=%.4f Volk-2h=%.4f\n",
        rms(nativeSquare),rms(rifleSquare),rms(rifleSupportedSquare),
        rms(volkSquare),rms(volkSupportedSquare));
    assert(rms(rifleSquare)<rms(nativeSquare)*.72);
    assert(rms(rifleSupportedSquare)<rms(rifleSquare)*.52);
    assert(rms(volkSquare)<rms(nativeSquare)*.87);
    // Native center and range remain invariant after measured-barrel alignment.
    float s[3]{1,2,3},e[3]{1001,2,3};const float from[3]{1,0,0},to[3]{0,1,0};
    assert(weapontrace::Align(s,e,from,to));
    assert(std::fabs(e[1]-1002)<.001f && s[0]==1 && s[1]==2 && s[2]==3);
}
