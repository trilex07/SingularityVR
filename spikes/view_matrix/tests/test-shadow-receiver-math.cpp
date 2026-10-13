#include "../shadow_receiver_math.h"
#include <cassert>
#include <cstdio>
#include <limits>
#include <iostream>
static void Transform(const float* p,const float* m,float* out) {
    for(int c=0;c<4;++c){double v=0;for(int r=0;r<4;++r)v+=(double)p[r]*m[r*4+c];out[c]=(float)v;}
}
int main(int argc,char**) {
    using namespace shadowreceiver;
    if(argc>1) {
        float vp[16],world[16],shadow[16],nw[16],ns[16],p[4]={0,0,0,1};
        for(auto& v:vp)std::cin>>v;for(auto& v:world)std::cin>>v;for(auto& v:shadow)std::cin>>v;
        std::cin>>p[0]>>p[1]>>p[2];assert(std::cin && Correct(vp,world,shadow,nw,ns));
        float q[4],before[4],after[4];Transform(p,vp,q);q[2]=q[3];q[3]=1;
        Transform(q,world,before);Transform(q,nw,after);
        double eb=0,ea=0;for(int c=0;c<3;++c){eb+=std::pow(before[c]-p[c],2);ea+=std::pow(after[c]-p[c],2);}
        printf("captured receiver reconstruction error before=%.6g after=%.6g units\n",std::sqrt(eb),std::sqrt(ea));
        return ea<.0001?0:1;
    }
    float oldWorld[16]={3,0,0,0, 0,2,0,0, 0,0,-1,0, 12,-4,30,1};
    float light[16]={.1f,0,0,.01f, 0,.2f,0,.02f, 0,0,.3f,.03f, .5f,.6f,.7f,1};
    float oldShadow[16];Multiply(oldWorld,light,oldShadow);
    unsigned samples=0;
    for(int eye=0;eye<2;++eye) {
        const float bias=eye?.5f:-.5f;
        float vp[16]={.5f,0,-bias,bias, 0,.7f,0,0, 0,0,-1.001f,-1, -.032f*(eye?1:-1),0,-.1001f,0};
        float nw[16],ns[16];assert(Correct(vp,oldWorld,oldShadow,nw,ns));
        for(float x:{-2.f,0.f,3.f})for(float y:{-1.f,0.f,2.f})for(float z:{-1.f,-10.f,-80.f}) {
            float p[4]={x,y,z,1},q[4],got[4],expectedLight[4],gotLight[4];
            Transform(p,vp,q);q[2]=q[3];q[3]=1;Transform(q,nw,got);
            Transform(p,light,expectedLight);Transform(q,ns,gotLight);
            for(int c=0;c<4;++c){assert(std::fabs(got[c]-p[c])<1e-4f);assert(std::fabs(gotLight[c]-expectedLight[c])<1e-4f);}
            ++samples;
        }
    }
    float zero[16]{},out[16];assert(!Inverse(zero,out));
    oldWorld[0]=std::numeric_limits<float>::quiet_NaN();assert(!Correct(light,oldWorld,light,out,zero));
    printf("PASS: %u per-eye reconstruction/world-light invariants; singular/nonfinite reject\n",samples);
}
