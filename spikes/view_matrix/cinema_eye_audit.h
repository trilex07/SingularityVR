#pragma once
#include "cinema_world_screen.h"
#include <algorithm>
#include <cmath>
namespace cinemaeyeaudit {
struct Bounds {float left=0,right=0,down=0,up=0;bool outsideX=false,outsideY=false,behind=false;};
inline bool Project(const cinemascreen::Pose& eye,const cinemascreen::Pose& screen,
                    float width,float height,const cinemascreen::Fov& fov,Bounds& out){
    if(!std::isfinite(width)||!std::isfinite(height)||width<=0||height<=0)return false;
    const float n=screen.qx*screen.qx+screen.qy*screen.qy+screen.qz*screen.qz+screen.qw*screen.qw;
    if(!std::isfinite(n)||n<.25f||n>4)return false;
    const float inv=1.f/std::sqrt(n),x=screen.qx*inv,y=screen.qy*inv,z=screen.qz*inv,w=screen.qw*inv;
    Bounds staged;bool first=true;
    for(int sx:{-1,1})for(int sy:{-1,1}){
        const float vx=sx*width*.5f,vy=sy*height*.5f;
        const float tx=-2*z*vy,ty=2*z*vx,tz=2*(x*vy-y*vx);
        const float wx=vx+w*tx+y*tz-z*ty,wy=vy+w*ty+z*tx-x*tz,wz=w*tz+x*ty-y*tx;
        float p[3];if(!cinemascreen::EyePoint(eye,screen.x+wx,screen.y+wy,screen.z+wz,p))return false;
        if(p[2]>=0)staged.behind=true;
        const float ax=std::atan2(p[0],-p[2]),ay=std::atan2(p[1],-p[2]);
        if(first){staged.left=staged.right=ax;staged.down=staged.up=ay;first=false;}
        else {staged.left=std::min(staged.left,ax);staged.right=std::max(staged.right,ax);
              staged.down=std::min(staged.down,ay);staged.up=std::max(staged.up,ay);}
    }
    staged.outsideX=staged.left<std::atan(fov.left)||staged.right>std::atan(fov.right);
    staged.outsideY=staged.down<std::atan(fov.down)||staged.up>std::atan(fov.up);
    out=staged;return true;
}
struct Gate {
    unsigned stable=0;bool done=false;
    bool Ready(bool enabled,bool valid){
        if(!enabled||!valid){stable=0;return false;}
        if(done)return false;
        if(++stable<30)return false;
        done=true;return true;
    }
};
}
