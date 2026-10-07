#pragma once
#include <cmath>
#include <cstring>
#include <cstdint>

// Row-vector affine matrices; GPU bone constants are three output-component rows.
namespace skeletal {
inline void Multiply(float out[4][4], const float a[4][4], const float b[4][4]) {
    float result[4][4]{};
    for (int i=0;i<4;++i) for(int j=0;j<4;++j) for(int k=0;k<4;++k) result[i][j]+=a[i][k]*b[k][j];
    memcpy(out,result,sizeof(result));
}
inline bool Inverse(float out[4][4], const float m[4][4]) {
    double a[4][8]{};
    for(int i=0;i<4;++i) for(int j=0;j<4;++j) { if(!std::isfinite(m[i][j]))return false; a[i][j]=m[i][j]; a[i][j+4]=i==j; }
    for(int k=0;k<4;++k) {
        int p=k; for(int i=k+1;i<4;++i) if(fabs(a[i][k])>fabs(a[p][k]))p=i;
        if(fabs(a[p][k])<1e-8)return false;
        for(int j=0;j<8;++j) {double t=a[k][j];a[k][j]=a[p][j];a[p][j]=t;}
        const double d=a[k][k];for(int j=0;j<8;++j)a[k][j]/=d;
        for(int i=0;i<4;++i) if(i!=k) {const double f=a[i][k];for(int j=0;j<8;++j)a[i][j]-=f*a[k][j];}
    }
    for(int i=0;i<4;++i)for(int j=0;j<4;++j)out[i][j]=(float)a[i][j+4];return true;
}
inline bool LocalDelta(float out[4][4],const float local[4][4],const float primary[4][4],const float off[4][4]) {
    float invP[4][4],invL[4][4],a[4][4],b[4][4];
    if(!Inverse(invP,primary)||!Inverse(invL,local))return false;
    Multiply(a,local,off);Multiply(b,a,invP);Multiply(out,b,invL);return true;
}
// Both hands share local/current; invert each once and commit only finite outputs.
inline bool LocalDeltaPair(float leftOut[4][4],float rightOut[4][4],const float local[4][4],const float current[4][4],const float left[4][4],const float right[4][4]) {
    float invCurrent[4][4],invLocal[4][4],a[4][4],b[4][4],leftResult[4][4],rightResult[4][4];
    if(!Inverse(invCurrent,current)||!Inverse(invLocal,local))return false;
    Multiply(a,local,left);Multiply(b,a,invCurrent);Multiply(leftResult,b,invLocal);
    Multiply(a,local,right);Multiply(b,a,invCurrent);Multiply(rightResult,b,invLocal);
    for(int i=0;i<4;++i)for(int j=0;j<4;++j)if(!std::isfinite(leftResult[i][j])||!std::isfinite(rightResult[i][j]))return false;
    memcpy(leftOut,leftResult,sizeof(leftResult));memcpy(rightOut,rightResult,sizeof(rightResult));return true;
}
inline void TransformBone(float out[3][4],const float bone[3][4],const float delta[4][4]) {
    float result[3][4]{};
    for(int component=0;component<3;++component) for(int input=0;input<4;++input) {
        for(int k=0;k<3;++k)result[component][input]+=bone[k][input]*delta[k][component];
        if(input==3)result[component][input]+=delta[3][component];
    }
    memcpy(out,result,sizeof(result));
}
struct Layout { int bones=-1, boneRegs=0, local=-1; bool shadow=false; };
// D3D9 CTAB offsets are relative to the header immediately after the FOURCC.
inline Layout ReadLayout(const uint32_t* words,unsigned count) {
    Layout result;
    for(unsigned at=1;at<count;) {
        const uint32_t token=words[at];
        if((token&0xffff)==0xffff)break;
        if((token&0xffff)!=0xfffe) {at+=1+((token>>24)&15);continue;}
        const unsigned n=(token>>16)&0x7fff;
        if(n>count-at-1)break;
        if(n>=8 && words[at+1]==0x42415443) {
            const unsigned char* base=(const unsigned char*)(words+at+2);const unsigned bytes=(n-1)*4;
            auto u32=[&](unsigned off){uint32_t v=0;if(off+4<=bytes)memcpy(&v,base+off,4);return v;};
            auto u16=[&](unsigned off){uint16_t v=0;if(off+2<=bytes)memcpy(&v,base+off,2);return v;};
            const unsigned constants=u32(12),info=u32(16);
            if(constants>256 || info>bytes || constants>(bytes-info)/20)break;
            bool projection=false,depth=false;
            for(unsigned i=0;i<constants;++i) {
                const unsigned p=info+i*20,name=u32(p);if(name>=bytes)continue;
                const char* text=(const char*)base+name;if(!memchr(text,0,bytes-name))continue;
                if(u16(p+4)!=2)continue; // FLOAT4 register set
                if(!strcmp(text,"BoneMatrices")){result.bones=u16(p+6);result.boneRegs=u16(p+8);}
                if(!strcmp(text,"LocalToWorld"))result.local=u16(p+6);
                if(!strcmp(text,"ProjectionMatrix"))projection=true;
                if(!strcmp(text,"InvMaxSubjectDepth")||!strcmp(text,"DepthBias"))depth=true;
            }
            result.shadow=projection&&depth;return result;
        }
        at+=n+1;
    }
    return result;
}
}
