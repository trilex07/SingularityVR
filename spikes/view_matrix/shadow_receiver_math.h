#pragma once
#include <cmath>
#include <algorithm>
namespace shadowreceiver {
inline bool Finite(const float* a){for(int i=0;i<16;++i)if(!std::isfinite(a[i]))return false;return true;}
inline void Multiply(const float* a,const float* b,float* out) {
    float m[16]{};
    for(int r=0;r<4;++r)for(int c=0;c<4;++c) {
        double v=0;for(int k=0;k<4;++k)v+=(double)a[r*4+k]*b[k*4+c];m[r*4+c]=(float)v;
    }
    std::copy(m,m+16,out);
}
inline bool Inverse(const float* in,float* out) {
    if(!Finite(in))return false;
    double a[4][8]{};
    for(int r=0;r<4;++r)for(int c=0;c<4;++c){a[r][c]=in[r*4+c];a[r][c+4]=r==c?1.:0.;}
    for(int c=0;c<4;++c) {
        int pivot=c;for(int r=c+1;r<4;++r)if(std::fabs(a[r][c])>std::fabs(a[pivot][c]))pivot=r;
        if(std::fabs(a[pivot][c])<1e-12)return false;
        for(int k=0;k<8;++k)std::swap(a[c][k],a[pivot][k]);
        const double divisor=a[c][c];for(int k=0;k<8;++k)a[c][k]/=divisor;
        for(int r=0;r<4;++r)if(r!=c){const double s=a[r][c];for(int k=0;k<8;++k)a[r][k]-=s*a[c][k];}
    }
    for(int r=0;r<4;++r)for(int c=0;c<4;++c)out[r*4+c]=(float)a[r][c+4];
    return Finite(out);
}
// Verified receiver VS copies actual clip position into its PS varying. PS
// reconstructs [clipX,clipY,linearW,1], NOT conventional [clipX,clipY,clipZ,W].
// Derive its inverse from the actual per-eye VS camera; preserve world->light.
inline bool Correct(const float* actualVP,const float* oldWorld,const float* oldShadow,
                    float* newWorld,float* newShadow) {
    if(!Finite(actualVP)||!Finite(oldWorld)||!Finite(oldShadow))return false;
    float screen[16],canonical[16],oldInverse[16],worldToShadow[16];
    for(int r=0;r<4;++r) {
        screen[r*4]=actualVP[r*4];screen[r*4+1]=actualVP[r*4+1];
        screen[r*4+2]=actualVP[r*4+3];screen[r*4+3]=r==3?1.f:0.f;
    }
    // This PS uses only XYZ from ScreenToWorld. Its otherwise-unused W column
    // must already be approximately affine; reject unknown/projective layouts.
    if(std::fabs(oldWorld[3])>1e-4f||std::fabs(oldWorld[7])>1e-4f||
       std::fabs(oldWorld[11])>1e-4f||std::fabs(oldWorld[15]-1.f)>1e-4f)return false;
    std::copy(oldWorld,oldWorld+16,canonical);
    canonical[3]=canonical[7]=canonical[11]=0;canonical[15]=1;
    if(!Inverse(screen,newWorld)||!Inverse(canonical,oldInverse))return false;
    Multiply(oldInverse,oldShadow,worldToShadow);
    Multiply(newWorld,worldToShadow,newShadow);
    return Finite(newWorld)&&Finite(newShadow);
}
}
