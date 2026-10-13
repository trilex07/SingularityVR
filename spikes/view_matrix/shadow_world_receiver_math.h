#pragma once
#include "shadow_receiver_math.h"
namespace worldshadowmath {
inline bool WorldDepth(const float* depth) {
    for(unsigned i=0;i<4;++i)if(!std::isfinite(depth[i]))return false;
    if(depth[0]<=0 || depth[1]<0 || depth[1]>=1)return false;
    // Only the near10 world family actually captured in PID35208. The
    // first-person near1 family is a different contract and stays untouched.
    const float nearPlane=depth[0]/(1-depth[1]);
    return std::fabs(nearPlane-10.f)<0.05f &&
        std::fabs(depth[2]*depth[0]-1.f)<1e-4f &&
        std::fabs(depth[3]*depth[0]-depth[1])<1e-5f;
}
inline bool Correct(const float* vp,const float* nativeWorld,const float* nativeShadow,
                    float* world,float* shadow) {
    if(!shadowreceiver::Finite(vp)||!shadowreceiver::Finite(nativeWorld)||
       !shadowreceiver::Finite(nativeShadow))return false;
    float screen[16],canonical[16],inverseNative[16],light[16];
    for(unsigned r=0;r<4;++r) {
        screen[r*4]=vp[r*4];screen[r*4+1]=vp[r*4+1];
        screen[r*4+2]=vp[r*4+3];screen[r*4+3]=r==3?1.f:0.f;
    }
    std::copy(nativeWorld,nativeWorld+16,canonical);
    // Audited PS28e01133 uses only XYZ here; native unused W can be1 or10.
    canonical[3]=canonical[7]=canonical[11]=0;canonical[15]=1;
    if(!shadowreceiver::Inverse(screen,world)||
       !shadowreceiver::Inverse(canonical,inverseNative))return false;
    shadowreceiver::Multiply(inverseNative,nativeShadow,light);
    shadowreceiver::Multiply(world,light,shadow);
    return shadowreceiver::Finite(world)&&shadowreceiver::Finite(shadow);
}
}
