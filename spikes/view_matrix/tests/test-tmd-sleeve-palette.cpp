#include "../tmd_sleeve_palette.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <initializer_list>

static void Identity(float m[4][4]) {
    std::memset(m,0,sizeof(float)*16);
    for(int i=0;i<4;++i)m[i][i]=1.f;
}
static void Near(const float a[3][4],const float b[3][4]) {
    for(int i=0;i<3;++i)for(int j=0;j<4;++j)
        assert(std::fabs(a[i][j]-b[i][j])<.0001f);
}
int main() {
    assert(tmdsleeve::CommitAllowed(true,true,true,45));
    assert(!tmdsleeve::CommitAllowed(false,true,true,45)); // GPU bind failed
    assert(!tmdsleeve::CommitAllowed(true,false,true,45)); // tracking lost
    assert(!tmdsleeve::CommitAllowed(true,true,false,45)); // palette rejected
    assert(!tmdsleeve::CommitAllowed(true,true,true,0)); // no independent draw
    for(int degrees : {-70,55}) {
        float original[45][4]{},hardware[45][4]{},staged[45][4]{},delta[4][4],identity[4][4];
        Identity(identity);Identity(delta);
        const float r=degrees*3.14159265358979323846f/180.f;
        delta[0][0]=delta[1][1]=std::cos(r);
        delta[0][1]=std::sin(r);delta[1][0]=-std::sin(r);
        delta[3][0]=13.f;delta[3][1]=-5.f;delta[3][2]=2.f;
        for(unsigned slot=0;slot<15;++slot) {
            original[slot*3][0]=original[slot*3+1][1]=original[slot*3+2][2]=1.f;
            original[slot*3][3]=float(slot)*.4f;
            original[slot*3+1][3]=-float(slot)*.2f;
            original[slot*3+2][3]=3.f;
        }
        std::memcpy(hardware,original,sizeof(original));
        // Slot 3 is animated tmd_base. Retargeting must preserve that source
        // animation, just as it preserves each sleeve bone's different pose.
        original[9][3]+=1.25f;
        std::memcpy(hardware,original,sizeof(original));
        for(unsigned slot=0;slot<15;++slot)
            if(tmdsleeve::kMap[slot]==27)
                skeletal::TransformBone(&hardware[slot*3],&original[slot*3],delta);
        assert(tmdsleeve::Stage(tmdsleeve::kMap,15,original,hardware,delta,staged));
        for(unsigned slot=0;slot<15;++slot) {
            bool sleeve=false;
            for(unsigned bone:tmdsleeve::kSleeveBones) sleeve|=tmdsleeve::kMap[slot]==bone;
            float expected[3][4];
            if(sleeve) skeletal::TransformBone(expected,&original[slot*3],delta);
            else std::memcpy(expected,&hardware[slot*3],sizeof(expected));
            Near(&staged[slot*3],expected);
            if(sleeve) {
                float twice[3][4];skeletal::TransformBone(twice,&staged[slot*3],delta);
                bool differs=false;
                for(int i=0;i<3;++i)for(int j=0;j<4;++j)
                    differs|=std::fabs(twice[i][j]-staged[slot*3+i][j])>.001f;
                assert(differs); // stage used original, not the already moved skin
            }
        }
        unsigned wrong[15];std::memcpy(wrong,tmdsleeve::kMap,sizeof(wrong));wrong[2]=9;
        assert(!tmdsleeve::Stage(wrong,15,original,hardware,delta,staged));
        assert(!tmdsleeve::Stage(tmdsleeve::kMap,14,original,hardware,delta,staged));
    }
    std::puts("PASS: material-1 map, two wrist rotations, native animation, seven sleeve bones, no double transform");
}
