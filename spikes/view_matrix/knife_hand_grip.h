#pragma once
// Joint assembly contract: original-rig pose + rigid knife mount. No engine
// offsets/hooks. Clip math reproduces the audited native row-matrix VS path.
#include "arm_bind.h"
#include "glove_animation.h"
#include <cstdint>

namespace knifehand {
struct Data {armbind::Mat mount[2];float fingers[2][5]{};};
inline bool Decode(const unsigned char* p,std::size_t bytes,Data& output){
    if(!p||bytes!=200||std::memcmp(p,"SVNG",4)||gloves::U32(p+4)!=1||
       gloves::U32(p+8)!=90||gloves::U32(p+12)!=2)return false;
    float header[4];std::memcpy(header,p+16,16);
    if(!std::isfinite(header[0])||header[0]<25||header[0]>200||header[1]!=75||header[2]!=52.5f||header[3]!=0)return false;
    Data staged;
    for(unsigned side=0;side<2;++side){
        std::memcpy(staged.mount[side].m,p+32+side*84,64);
        std::memcpy(staged.fingers[side],p+96+side*84,20);
        const auto& m=staged.mount[side];float scale=0;
        if(!armbind::Finite(m)||!gloves::UniformScale(m.m,scale)||std::fabs(scale-header[0])>.01f||
           std::fabs(m.m[0][3])+std::fabs(m.m[1][3])+std::fabs(m.m[2][3])+std::fabs(m.m[3][3]-1)>1e-5f||
           std::fabs(m.m[3][0])+std::fabs(m.m[3][1])+std::fabs(m.m[3][2])>500)return false;
        for(float f:staged.fingers[side])if(!std::isfinite(f)||f<0||f>1)return false;
    }
    output=staged;return true;
}
inline int Digit(unsigned bone){
    if(bone>=10&&bone<=12)return 0;if(bone>=23&&bone<=25)return 1;
    if(bone>=20&&bone<=22)return 2;if(bone>=17&&bone<=19)return 3;
    if(bone>=14&&bone<=16)return 4;
    if(bone>=70&&bone<=72)return 0;if(bone>=79&&bone<=81)return 1;
    if(bone>=76&&bone<=78)return 2;if(bone>=73&&bone<=75)return 3;
    if(bone>=82&&bone<=84)return 4;return -1;
}
struct ClipVertex {float clip[4],u,v;};
struct ScreenVertex {float x,y,z,rhw;std::uint32_t colour;float u,v;};
static_assert(sizeof(ScreenVertex)==28,"RHW original UV vertex");
struct Viewport {float x,y,width,height,minZ,maxZ;};
inline bool StandardDepth(const float m[4][4]){
    double zDotW=0,w2=0;
    for(unsigned r=0;r<3;++r){zDotW+=(double)m[r][2]*m[r][3];w2+=(double)m[r][3]*m[r][3];}
    // Audited standard UE3 perspective: Z ~= a*W+b, a > .5. Reversed/custom
    // projections must not silently consume a LESS/clear-1 private depth path.
    return std::isfinite(zDotW)&&std::isfinite(w2)&&w2>1e-10&&zDotW/w2>.5&&zDotW/w2<2.;
}
inline bool Transform(ClipVertex& out,gloves::V p,float u,float v,const float m[4][4]){
    for(unsigned c=0;c<4;++c){out.clip[c]=p.x*m[0][c]+p.y*m[1][c]+p.z*m[2][c]+m[3][c];
        if(!std::isfinite(out.clip[c]))return false;}
    out.u=u;out.v=v;return std::isfinite(u)&&std::isfinite(v);
}
inline float Plane(const ClipVertex& p,unsigned plane){return plane==0?p.clip[3]-.0001f:plane==1?p.clip[2]:p.clip[3]-p.clip[2];}
// Homogeneous near/far/w clipping. No whole-blade disappearance when its tip
// crosses the near plane. At most six polygon vertices / twelve output vertices.
inline unsigned ClipTriangle(const ClipVertex input[3],ClipVertex output[12]){
    ClipVertex a[8],b[8];std::memcpy(a,input,3*sizeof(ClipVertex));unsigned count=3;
    for(unsigned plane=0;plane<3;++plane){
        unsigned next=0;
        for(unsigned i=0;i<count;++i){const auto& x=a[i];const auto& y=a[(i+1)%count];
            const float dx=Plane(x,plane),dy=Plane(y,plane);const bool ix=dx>=0,iy=dy>=0;
            if(ix){if(next>=8)return 0;b[next++]=x;}
            if(ix!=iy){const float t=dx/(dx-dy);if(t<=0||t>=1)continue;if(next>=8)return 0;ClipVertex z;
                for(unsigned c=0;c<4;++c)z.clip[c]=x.clip[c]+t*(y.clip[c]-x.clip[c]);
                z.u=x.u+t*(y.u-x.u);z.v=x.v+t*(y.v-x.v);b[next++]=z;}
        }
        if(next<3)return 0;count=next;std::memcpy(a,b,count*sizeof(ClipVertex));
    }
    if(count>6)return 0;
    unsigned n=0;for(unsigned i=1;i+1<count;++i){output[n++]=a[0];output[n++]=a[i];output[n++]=a[i+1];}return n;
}
inline bool Screen(const ClipVertex& p,const Viewport& vp,ScreenVertex& out){
    const float w=p.clip[3];if(!std::isfinite(w)||w<.000099f||vp.width<=0||vp.height<=0)return false;
    const float inverse=1/w;
    out={(p.clip[0]*inverse+1)*vp.width*.5f+vp.x,(1-p.clip[1]*inverse)*vp.height*.5f+vp.y,
         vp.minZ+p.clip[2]*inverse*(vp.maxZ-vp.minZ),inverse,0xffffffff,p.u,p.v};
    return std::isfinite(out.x)&&std::isfinite(out.y)&&std::isfinite(out.z);
}
} // namespace knifehand
