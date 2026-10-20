#pragma once
#include <cmath>
namespace weapontrace {
// Shortest rigid rotation from an existing aim axis to a measured barrel.
// Unlike spherical pitch addition this preserves the native spread cone and
// ray length even close to vertical. Antiparallel/invalid inputs refuse.
inline bool Align(float* start,float* end,const float* from,const float* to){
    if(!start||!end||!from||!to)return false;
    float a[3],b[3],d[3],aa=0,bb=0;
    for(unsigned i=0;i<3;++i){
        if(!std::isfinite(start[i])||!std::isfinite(end[i])||
           !std::isfinite(from[i])||!std::isfinite(to[i]))return false;
        a[i]=from[i];b[i]=to[i];d[i]=end[i]-start[i];aa+=a[i]*a[i];bb+=b[i]*b[i];
    }
    if(!std::isfinite(aa)||!std::isfinite(bb)||aa<.0001f||bb<.0001f)return false;
    aa=std::sqrt(aa);bb=std::sqrt(bb);
    float c=0;for(unsigned i=0;i<3;++i){a[i]/=aa;b[i]/=bb;c+=a[i]*b[i];}
    if(c<-.9999f)return false;
    const float v[3]{a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};
    const float vd=v[0]*d[0]+v[1]*d[1]+v[2]*d[2];
    float result[3];
    for(unsigned i=0;i<3;++i){
        const unsigned j=(i+1)%3,k=(i+2)%3;
        result[i]=start[i]+c*d[i]+v[j]*d[k]-v[k]*d[j]+v[i]*vd/(1.f+c);
        if(!std::isfinite(result[i]))return false;
    }
    for(unsigned i=0;i<3;++i)end[i]=result[i];return true;
}
// Relocating only Start changes atan2(End-Start), especially near a target.
// Translate the entire finite ray, retaining its spread and original length.
inline bool Translate(float* start,float* end,const float* offset){
    if(!start||!end||!offset)return false;
    float a[3],b[3];
    for(unsigned i=0;i<3;++i){a[i]=start[i]+offset[i];b[i]=end[i]+offset[i];
        if(!std::isfinite(a[i])||!std::isfinite(b[i]))return false;}
    for(unsigned i=0;i<3;++i){start[i]=a[i];end[i]=b[i];}return true;
}
}
