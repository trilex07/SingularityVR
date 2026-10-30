#include "../singularity_bind_reference.h"
#include <cassert>
#include <cmath>
#include <cstdio>
int main(){
    // Verify NATIVE wrist axes from the full named reference hierarchy.
    // A PSK child's positive local Y is reflected before native composition.
    for(const auto* refs:{bindassets::noTmd,bindassets::tmd}){
        armbind::Mat bind[90],inverse,elbow,index;
        assert(armbind::BuildReference(refs,refs==bindassets::noTmd?90:89,bind));
        assert(skeletal::Inverse(inverse.m,bind[9].m));
        skeletal::Multiply(elbow.m,bind[8].m,inverse.m); // l_arm_elbow
        skeletal::Multiply(index.m,bind[23].m,inverse.m); // l_index_1
        assert(elbow.m[3][1]>21.f&&index.m[3][1]<-8.f);
    }
    // Source wrist-to-elbow length, not its sign in cooked wrist coordinates.
    const float forearmM=bindassets::noTmd[9].pskPosition.y/52.5f;
    assert(forearmM>.40f&&forearmM<.42f);
    assert(std::fabs(bindassets::tmd[9].pskPosition.y-
                     bindassets::noTmd[9].pskPosition.y)<.001f);
    const float length=.155f,width=.104f,centreFromWrist=.080f;
    const float nearEnd=centreFromWrist-length*.5f;
    const float farEnd=centreFromWrist+length*.5f;
    assert(nearEnd>=0.f&&farEnd<forearmM*.5f);
    assert(width<.11f);
    std::printf("PASS source wrist-to-elbow %.1f mm; panel forearm %.1f..%.1f mm, cross-width %.1f mm\n",
                forearmM*1000,nearEnd*1000,farEnd*1000,width*1000);
}
