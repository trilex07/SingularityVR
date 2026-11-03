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
    {
        const cinemascreen::Pose eyes[2]={{-.032f,0,0,0,0,0,1},{.032f,0,0,0,0,0,1}};
        const cinemascreen::Fov fovs[2]={{-1.2f,.7f,-.8f,.8f},{-.5f,1.3f,-.65f,.9f}};
        const auto stereoHead=cinemascreen::Centre(eyes[0],eyes[1]);
        cinemascreen::Anchor fitted;
        assert(fitted.LatchStereo(stereoHead,2,1.3f,.9f,16.f/9,eyes,fovs));
        assert(fitted.width<2*2*1.3f*.9f); // left-eye width alone clips the RIGHT eye
        for(unsigned eye=0;eye<2;++eye)for(int x:{-1,1})for(int y:{-1,1}){
            float p[3];assert(cinemascreen::EyePoint(eyes[eye],x*fitted.width*.5f,y*fitted.height*.5f,-2,p));
            const float tx=p[0]/-p[2],ty=p[1]/-p[2];
            assert(tx>=fovs[eye].left-1e-5f&&tx<=fovs[eye].right+1e-5f);
            assert(ty>=fovs[eye].down-1e-5f&&ty<=fovs[eye].up+1e-5f);
        }
        cinemascreen::Anchor swapped;const cinemascreen::Pose reverseEyes[2]={eyes[1],eyes[0]};
        const cinemascreen::Fov reverseFov[2]={fovs[1],fovs[0]};
        assert(swapped.LatchStereo(stereoHead,2,1.3f,.9f,16.f/9,reverseEyes,reverseFov));
        assert(near(swapped.width,fitted.width)&&near(swapped.height,fitted.height));
        assert(near(fitted.width/fitted.height,16.f/9));
        const float lockedWidth=fitted.width;
        assert(fitted.LatchStereo(stereoHead,2,4,1,1,eyes,fovs)&&fitted.width==lockedWidth);
        cinemascreen::Fov invalid[2]={fovs[0],fovs[1]};invalid[1].right=0;
        cinemascreen::Anchor bad;assert(!bad.LatchStereo(stereoHead,2,1,.6f,2,eyes,invalid)&&!bad.valid);
        float rotated[3];const cinemascreen::Pose yawEye{0,0,0,0,.70710678f,0,.70710678f};
        assert(cinemascreen::EyePoint(yawEye,-2,0,0,rotated));
        assert(std::fabs(rotated[0])<1e-5f&&std::fabs(rotated[2]+2)<1e-5f);
        puts("PASS shared cinema bounds: asymmetric eye FOV/IPD, vertical clip, eye-order parity, aspect, latch and invalid refusal");
    }
    {
        // Reproduce the observed border disparity: equal physical quad sizes
        // do NOT imply equal eye-relative edges at 64 mm IPD / 2 m distance.
        const cinemascreen::Pose eyes[2]={{-.032f,0,0,0,0,0,1},{.032f,0,0,0,0,0,1}};
        const cinemascreen::Pose anchor{0,0,-2,0,0,0,1};
        float oldL[3],oldR[3];
        assert(cinemascreen::EyePoint(eyes[0],1,0,-2,oldL));
        assert(cinemascreen::EyePoint(eyes[1],1,0,-2,oldR));
        assert(std::fabs(oldL[0]/-oldL[2]-oldR[0]/-oldR[2])>.03f);
        for(int step=0;step<12;++step){
            const float yaw=step*.17f,dx=.032f*std::cos(yaw),dz=-.032f*std::sin(yaw);
            const float cx=step*.07f,cy=step*.02f,cz=-step*.03f;
            const cinemascreen::Pose moved[2]={
                {cx-dx,cy,cz-dz,0,std::sin(yaw*.5f),0,std::cos(yaw*.5f)},
                {cx+dx,cy,cz+dz,0,std::sin(yaw*.5f),0,std::cos(yaw*.5f)}};
            cinemascreen::Pose screens[2];
            for(unsigned e=0;e<2;++e)assert(cinemascreen::BinocularFramePose(anchor,moved,e,screens[e]));
            for(int x:{-1,1})for(int y:{-1,1}){
                float a[3],b[3];
                assert(cinemascreen::EyePoint(moved[0],screens[0].x+x,screens[0].y+y*.5f,screens[0].z,a));
                assert(cinemascreen::EyePoint(moved[1],screens[1].x+x,screens[1].y+y*.5f,screens[1].z,b));
                for(unsigned k=0;k<3;++k)assert(near(a[k],b[k]));
            }
            assert(screens[0].qy==anchor.qy&&screens[1].qw==anchor.qw);
            assert(near((screens[0].x+screens[1].x)*.5f,anchor.x));
        }
        cinemascreen::Pose output=anchor;const cinemascreen::Pose invalid[2]={eyes[0],eyes[0]};
        assert(!cinemascreen::BinocularFramePose(anchor,invalid,0,output)&&output.z==anchor.z);
        assert(!cinemascreen::BinocularFramePose(anchor,eyes,2,output));
        puts("PASS cinema frame: observed IPD border disparity reproduced; both-eye corner parity under head/turn motion, fixed cyclopean anchor, invalid refusal");
    }
    std::puts("PASS: cinema LOCAL anchor stable under motion, aspect, pause/exit/new-scene lifecycle, invalid tracking fail-closed");
}
