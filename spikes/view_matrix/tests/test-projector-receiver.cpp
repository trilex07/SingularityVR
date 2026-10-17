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
}
