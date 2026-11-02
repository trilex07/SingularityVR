#pragma once
#include <cmath>
#include <cstdint>

namespace cinemamutant {
// Adapter supplies reflected native identities, not proximity-based enemies.
template<class Scene> uintptr_t InteractionAttacker(Scene& scene, uintptr_t pawn) {
    if (!pawn || !scene.PlayerInteraction(pawn)) return 0;
    const uintptr_t instigator=scene.Instigator(pawn);
    return instigator && scene.Attacking(instigator,pawn) ? instigator:0;
}
inline bool Redirect(bool enabled, bool nativeCinema, bool fireWindow,
                     uintptr_t source, uintptr_t pawn, uintptr_t target,
                     const float* start, const float* end, const float* center, float* out) {
    if (!enabled || !nativeCinema || !fireWindow || !pawn || source!=pawn || !target ||
        !start || !end || !center || !out) return false;
    double range2=0,distance2=0;
    for(int i=0;i<3;++i) {
        if(!std::isfinite(start[i])||!std::isfinite(end[i])||!std::isfinite(center[i]))return false;
        const double ray=double(end[i])-start[i], aim=double(center[i])-start[i];
        range2+=ray*ray; distance2+=aim*aim;
    }
    // Never extend the native shot's reach or aim through a coincident origin.
    // Four actual player SLC receipts in this interaction have 8000 UU range.
    // Exclude short obstruction/physics traces inside the same fire window.
    const double range=std::sqrt(range2);
    if(distance2<=0.0001 || range2<distance2 || std::abs(range-8000.0)>2.0)return false;
    const double scale=std::sqrt(range2/distance2);
    for(int i=0;i<3;++i)out[i]=float(start[i]+(double(center[i])-start[i])*scale);
    return true;
}
}
