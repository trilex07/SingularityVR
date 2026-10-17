#include "../projector_receiver_math.h"
#include <cassert>
#include <limits>
static void Point(const float* p,const float* m,float* out){for(int c=0;c<4;++c){out[c]=0;for(int r=0;r<4;++r)out[c]+=p[r]*m[r*4+c];}}
int main(){
    const float native[16]={1.2f,0,0,0, 0,1.5f,0,0, 0,0,1,1, 0,0,-10,0};
    const float light[16]={.003f,0,.001f,0, 0,-.002f,0,0, -.0004f,0,.0009f,0, -.012f,0,.168f,1};
    float originalScreen[16],same[16];projectormath::Screen(native,originalScreen);
    assert(projectormath::Correct(native,native,light,same));
    for(int i=0;i<16;++i)assert(std::fabs(same[i]-light[i])<1e-6f);
    for(int eye=0;eye<2;++eye){
        float vp[16];std::copy(native,native+16,vp);
        // Different FOV, eye translation, and clip half remap.
        vp[0]=.9f;vp[5]=1.1f;vp[12]=eye?-.065f:0;
        for(int r=0;r<4;++r)vp[4*r]=vp[4*r]*.5f+(eye?.5f:-.5f)*vp[4*r+3];
        float corrected[16],screen[16];projectormath::Screen(vp,screen);
        assert(projectormath::Correct(native,vp,light,corrected));
        for(int z=1;z<6;++z){float world[4]={2.f,-3.f,20.f*z,1},s[4],n[4],expected[4],actual[4];
            Point(world,screen,s);Point(world,originalScreen,n);Point(n,light,expected);Point(s,corrected,actual);
            for(int c=0;c<4;++c)assert(std::fabs(actual[c]-expected[c])<1e-5f);
        }
    }
    float invalid[16]{};assert(!projectormath::Correct(native,invalid,light,same));
    invalid[0]=std::numeric_limits<float>::quiet_NaN();assert(!projectormath::Correct(invalid,native,light,same));
    // Recorded PID15548 groups171/172. Validate eye-to-eye invariance using the
    // actual camera/light constants, independently of the missing native VP.
    // This proves the transform identity, not absolute placement on the canvas.
    const float left[16]={-.608929157f,.0155705884f,.922247648f,.923170805f,
        .162987962f,.0603301711f,.380764961f,.381146103f,
        -.000469023362f,.749872088f,-.0497838929f,-.0498337261f,
        .118439734f,-.54312712f,-8.92222023f,1.06884885f};
    const float right[16]={.314241648f,.0155705884f,.922247648f,.923170805f,
        .54413408f,.0603301711f,.380764961f,.381146103f,
        -.0503027514f,.749872088f,-.0497838929f,-.0498337261f,
        -.449118376f,-.54309082f,-8.9200983f,1.07097304f};
    const float recordedLight[16]={.00283966935f,.00000402826117f,.00117985404f,0,
        -.0000390042005f,-.00156513753f,.0000992188725f,0,
        -.000373972754f,.0000663529063f,.000899704173f,0,
        -.0119514465f,-.000240072608f,.168351173f,.999999821f};
    float sl[16],sr[16],fixed[16];projectormath::Screen(left,sl);projectormath::Screen(right,sr);
    assert(projectormath::Correct(left,right,recordedLight,fixed));
    float maxUncorrected=0;
    for(int z=20;z<=500;z+=20){
        float p[4]={2,-3,(float)z,1},a[4],b[4],expected[4],actual[4],bad[4];
        Point(p,sl,a);Point(p,sr,b);Point(a,recordedLight,expected);
        Point(b,fixed,actual);Point(b,recordedLight,bad);
        for(int c=0;c<3;++c){assert(std::fabs(actual[c]-expected[c])<1e-5f);
            maxUncorrected=std::max(maxUncorrected,std::fabs(bad[c]-expected[c]));}
    }
    assert(maxUncorrected>.01f);
}
