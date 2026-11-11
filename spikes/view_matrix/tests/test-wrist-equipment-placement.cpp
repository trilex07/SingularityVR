#include "../singularity_bind_reference.h"
#include <cassert>
#include <cmath>
#include <cstdio>
int main(){
    // Source bind: wrist is +21.4858837 UU from elbow along forearm Y.
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
