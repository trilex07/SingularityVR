#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>

namespace cameracomfort {
struct Pov { float location[3]; std::int32_t rotation[3]; float fov; };
static_assert(sizeof(Pov)==28,"UE3 FPOV layout");
struct Gates {
    bool enabled=false,vr=false,gameplay=false,cinematic=false,menu=false;
    bool sameCamera=false,samePawn=false,overridden=false;
    unsigned style=0;
    float cameraBlend=0;
};
inline bool Eligible(const Gates& g) {
    return g.enabled && g.vr && g.gameplay && !g.cinematic && !g.menu &&
        g.sameCamera && g.samePawn && !g.overridden && g.style==1 &&
        std::isfinite(g.cameraBlend) && g.cameraBlend==0;
}
// World-space engine output, NOT a shader-space compensation. Both the outgoing
// view and the mod's existing camera-origin source receive the same position.
// No HMD/controller pose, inventory, input or native modifier state is edited.
inline bool Reduce(const Gates& gates,const float pawn[3],float baseEye,
                   const Pov& unshaken,Pov& outgoing,Pov& anchor) {
    if(!Eligible(gates) || !std::isfinite(baseEye) || baseEye<=0 || baseEye>200)
        return false;
    for(int i=0;i<3;++i)
        if(!std::isfinite(pawn[i]) || std::fabs(pawn[i])>1.e7f ||
           !std::isfinite(unshaken.location[i]) || !std::isfinite(outgoing.location[i]))
            return false;
    if(!std::isfinite(outgoing.fov) || outgoing.fov<=0 || outgoing.fov>=180) return false;
    Pov next=outgoing;
    for(int i=0;i<3;++i) {
        next.location[i]=pawn[i]+(i==2?baseEye:0);
        next.rotation[i]=unshaken.rotation[i];
    }
    // Do not copy FOV back: native FOV interpolation/VR projection owns it.
    std::memcpy(anchor.location,next.location,sizeof(next.location));
    std::memcpy(outgoing.location,next.location,sizeof(next.location));
    std::memcpy(outgoing.rotation,next.rotation,sizeof(next.rotation));
    return true;
}
}
