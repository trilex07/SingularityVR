#pragma once
#include "shadow_receiver_math.h"
namespace projectormath {
// Audited PS7F00BC79 consumes [NDC.x*linearDepth,NDC.y*linearDepth,linearDepth,1].
// Row-vector convention: retain native world->light, replace only screen->world.
inline void Screen(const float* vp,float* out){
    for(int r=0;r<4;++r){out[4*r]=vp[4*r];out[4*r+1]=vp[4*r+1];
        out[4*r+2]=vp[4*r+3];out[4*r+3]=r==3?1.f:0.f;}
}
inline bool Correct(const float* nativeVP,const float* eyeVP,const float* nativeLight,float* out){
    if(!shadowreceiver::Finite(nativeVP)||!shadowreceiver::Finite(eyeVP)||!shadowreceiver::Finite(nativeLight))return false;
    float nativeScreen[16],eyeScreen[16],inverse[16],worldLight[16];
    Screen(nativeVP,nativeScreen);Screen(eyeVP,eyeScreen);
    if(!shadowreceiver::Inverse(eyeScreen,inverse))return false;
    shadowreceiver::Multiply(nativeScreen,nativeLight,worldLight);
    shadowreceiver::Multiply(inverse,worldLight,out);
    return shadowreceiver::Finite(out);
}
}
