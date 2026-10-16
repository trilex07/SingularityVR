#include "../cinema_world_screen.h"
#include <cassert>
#include <cstdio>
#include <limits>
static bool near(float a,float b){return std::fabs(a-b)<1e-5f;}
int main(){
    auto centre=cinemascreen::Centre({-.032f,1,0,0,.087f,0,.996f},{.032f,1,0,0,-.087f,0,.996f});
    assert(near(centre.x,0) && near(centre.qy,0));
    centre=cinemascreen::Centre({0,1,0,0,0,0,1},{0,1,0,0,0,0,-1});
    assert(near(centre.qw,1)); // q and -q describe the same orientation
    assert(!cinemascreen::TrackingAllowsAnchor(false,false));
    assert(cinemascreen::TrackingAllowsAnchor(false,true));
    assert(cinemascreen::TrackingAllowsAnchor(true,false)); // retain stable screen through transient tracking loss
    cinemascreen::Anchor a;
    cinemascreen::Pose head{1,1.4f,2,0,0,0,1};
    assert(a.Latch(head,2,1,.6f,16.f/9));
    assert(a.valid && near(a.pose.x,1) && near(a.pose.y,1.4f) && near(a.pose.z,0));
    assert(near(a.width,2.4f) && near(a.width/a.height,16.f/9));
    const auto locked=a.pose;
    head.x=100;head.qy=.70710678f;head.qw=.70710678f;
    for(int frame=0;frame<1000;++frame){
        assert(a.Latch(head,2,1,.6f,16.f/9));
        assert(a.pose.x==locked.x && a.pose.z==locked.z && a.pose.qy==locked.qy);
    }
    a.Observe(true,true);assert(a.valid); // pause/menu while cinematic remains active
    a.Observe(false,true);assert(!a.valid); // no quad state leaks into gameplay
    // VideoFlat world anchor lifecycle
    {
        const bool isVideoFlat = true;
        const bool activeWorldAnchor = false || isVideoFlat;
        cinemascreen::Pose vhead{0,1.2f,0,0,0,0,1};
        assert(a.Latch(vhead,2,1,.6f,16.f/9));
        assert(a.valid);
        a.Observe(activeWorldAnchor, true);
        assert(a.valid); // stays latched during video playback
        a.Observe(false, true);
        assert(!a.valid); // resets cleanly when video ends
    }
    // MainMenu cinema world anchor lifecycle (MenuFlat + menuClass 1/2/4)
    {
        const bool isMainMenuFlat = true;
        const bool menuWorldScreen = true;
        const bool activeWorldAnchor = (menuWorldScreen && isMainMenuFlat);
        cinemascreen::Pose mhead{0,1.4f,0,0,0,0,1};
        assert(a.Latch(mhead,2,1,.6f,16.f/9));
        assert(a.valid);
        a.Observe(activeWorldAnchor, true);
        assert(a.valid); // stays latched while in main menu
        a.Observe(false, true); // transition to gameplay resets anchor
        assert(!a.valid);
    }

    assert(a.Latch(head,2,1,.6f,16.f/9));
    assert(near(a.pose.x,98) && near(a.pose.z,2)); // new scene chooses new anchor
    assert(near(a.pose.qy,.70710678f));
    a.Observe(true,false);assert(!a.valid);
    head.qy=0;head.qw=0;assert(!a.Latch(head,2,1,.6f,2));assert(!a.valid);
    head.qw=1;head.x=std::numeric_limits<float>::quiet_NaN();
    assert(!a.Latch(head,2,1,.6f,2));assert(!a.valid);
    head.x=0;head.qx=.70710678f;head.qw=.70710678f;
    assert(!a.Latch(head,2,1,.6f,2)); // vertical heading refuses rather than jumps
    head.qx=0;head.qw=1;
    assert(!a.Latch(head,-1,1,.6f,2));assert(!a.Latch(head,2,1,1.1f,2));
    assert(cinemascreen::ClampDistanceMm(-1)==800 && cinemascreen::ClampDistanceMm(2000)==2000 &&
           cinemascreen::ClampDistanceMm(20000)==10000);
    std::puts("PASS: cinema LOCAL anchor stable under motion, aspect, pause/exit/new-scene lifecycle, invalid tracking fail-closed");
}
