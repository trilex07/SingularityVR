#pragma once
#include "hip_knife.h"
#include <algorithm>
#include <cstdint>

// Controller presentation/aim only. NEVER feed an HMD pose through this filter.
namespace weaponsteady {
inline bool Hold(bool enabled,bool gameplay,bool tracked,bool sniper,bool twoHand,
                 bool primaryGrip,bool busy){
    return enabled&&gameplay&&tracked&&sniper&&twoHand&&primaryGrip&&!busy;
}
struct Filter {
    hipknife::Pose value{};
    std::uint32_t pawn=0,weapon=0;
    std::uint64_t tick=0;
    bool active=false,lastStrong=false;
    hipknife::Pose Update(hipknife::Pose raw,bool enabled,bool strong,
                         std::uint32_t p,std::uint32_t w,std::uint64_t now){
        const bool valid=hipknife::Finite(raw.p)&&hipknife::Valid(raw.q);
        const bool reset=!enabled||!valid||!active||lastStrong!=strong||pawn!=p||weapon!=w||
            !tick||now<=tick||now-tick>100;
        const float dt=reset?0.f:float(now-tick)*.001f;
        pawn=p;weapon=w;tick=now;lastStrong=strong;active=enabled&&valid&&p&&w;
        if(reset||!active){value=raw;return raw;}
        const float distance=hipknife::Length(raw.p-value.p);
        float dot=value.q.x*raw.q.x+value.q.y*raw.q.y+value.q.z*raw.q.z+value.q.w*raw.q.w;
        if(dot<0){raw.q={-raw.q.x,-raw.q.y,-raw.q.z,-raw.q.w};dot=-dot;}
        // Snap intentional fast moves, recenter/teleport and tracking discontinuities.
        const float angle=2.f*std::acos(std::min(1.f,dot));
        if(distance>.04f||angle>.20f){value=raw;return value;}
        const float motion=std::max(distance/.012f,angle/.08f);
        // R1 + supported Kasimov: suppress small physiological tremor, not just
        // alternating sensor noise. Both hands use this same response because
        // their POSITION difference is the supported barrel's aim axis.
        // Residual-adaptive gain and the snap above keep deliberate turns live.
        const float tau=(strong?.200f:.018f)/(1.f+motion*motion*3.f);
        const float alpha=1.f-std::exp(-dt/tau);
        value.p=value.p+(raw.p-value.p)*alpha;
        hipknife::Q q{value.q.x+(raw.q.x-value.q.x)*alpha,
            value.q.y+(raw.q.y-value.q.y)*alpha,value.q.z+(raw.q.z-value.q.z)*alpha,
            value.q.w+(raw.q.w-value.q.w)*alpha};
        const float n=std::sqrt(q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w);
        value.q={q.x/n,q.y/n,q.z/n,q.w/n};return value;
    }
};
}
