#include "../cinema_eye_audit.h"
#include <cassert>
#include <cstdio>
int main(){
    using namespace cinemascreen;
    constexpr float left=-1.07337749f,right=.75828403f;
    const Fov fovs[2]={{std::tan(left),std::tan(right),-1.328676f,1.328676f},
                      {-std::tan(right),-std::tan(left),-1.328676f,1.328676f}};
    const Pose eye{0,0,0,0,0,0,1};
    // Actual PSVR2 FOV: same geometric border at +50deg is clipped only in LEFT eye.
    const float x=2*std::tan(.872664626f);const Pose screen{x,0,-2,0,0,0,1};
    cinemaeyeaudit::Bounds b[2];
    for(unsigned e=0;e<2;++e)assert(cinemaeyeaudit::Project(eye,screen,.1f,.2f,fovs[e],b[e]));
    assert(b[0].outsideX&&!b[1].outsideX);
    assert(std::fabs(b[0].right-b[1].right)<1e-6f);
    const Pose centred{0,0,-2,0,0,0,1};
    assert(cinemaeyeaudit::Project(eye,centred,2,1,fovs[0],b[0]));
    assert(!b[0].outsideX&&!b[0].outsideY&&!b[0].behind);
    const Pose behind{0,0,2,0,0,0,1};
    assert(cinemaeyeaudit::Project(eye,behind,2,1,fovs[0],b[0])&&b[0].behind);
    assert(!cinemaeyeaudit::Project(eye,centred,-1,1,fovs[0],b[0]));
    cinemaeyeaudit::Gate gate;
    for(int i=0;i<29;++i)assert(!gate.Ready(true,true));
    assert(!gate.Ready(true,false));
    for(int i=0;i<29;++i)assert(!gate.Ready(true,true));
    assert(gate.Ready(true,true));for(int i=0;i<100;++i)assert(!gate.Ready(true,true));
    cinemaeyeaudit::Gate off;for(int i=0;i<100;++i)assert(!off.Ready(false,true));
    puts("PASS actual PSVR2 asymmetric-FOV classification, equal edge rays, behind/invalid rejection, one-shot stable/OFF gate");
}
