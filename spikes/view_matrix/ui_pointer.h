#pragma once
#include <cmath>
#include <algorithm>

namespace uipointer {
struct V { float x=0,y=0,z=0; };
inline V operator+(V a,V b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline V operator-(V a,V b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline V operator*(V a,float s){return {a.x*s,a.y*s,a.z*s};}
inline float Dot(V a,V b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline bool Finite(V a){return std::isfinite(a.x)&&std::isfinite(a.y)&&std::isfinite(a.z);}
inline bool Unit(V a,V& b){float n=std::sqrt(Dot(a,a));if(!std::isfinite(n)||n<1e-6f)return false;b=a*(1/n);return true;}
struct Plane { V centre,right,up,normal; float halfWidth=0,halfHeight=0,inset=.70f; };
// Basis is the SAME rendered view basis used to place the menu. All inputs share
// one coordinate space/units. Symmetric projection half-FOV tangents set extent.
inline bool HeadPlane(V head,V forward,V right,V up,float distance,float tanHalfX,float tanHalfY,float inset,Plane& out){
    if(!Finite(head)||!std::isfinite(distance)||distance<=0||!std::isfinite(tanHalfX)||!std::isfinite(tanHalfY)||tanHalfX<=0||tanHalfY<=0||!std::isfinite(inset)||inset<=0||inset>1)return false;
    Plane p;if(!Unit(forward,p.normal)||!Unit(right,p.right)||!Unit(up,p.up))return false;
    if(std::fabs(Dot(p.normal,p.right))>1e-3f||std::fabs(Dot(p.normal,p.up))>1e-3f||std::fabs(Dot(p.right,p.up))>1e-3f)return false;
    p.centre=head+p.normal*distance;p.halfWidth=distance*tanHalfX;p.halfHeight=distance*tanHalfY;p.inset=inset;
    if(!Finite(p.centre)||!std::isfinite(p.halfWidth)||!std::isfinite(p.halfHeight))return false;out=p;return true;
}
struct Hit { V world; float distance=0,u=0,v=0,eyeU=0,eyeV=0; int pixelX=0,pixelY=0; };
struct ScreenPoint { float u=0,v=0,depth=0; };
// Project a tracking-space point through the same symmetric per-eye view used by OpenXR.
// The result is local to one eye (0..1 on-screen). It deliberately does not know about SBS.
inline bool ProjectPoint(V point,V eye,V forward,V right,V up,float tanHalfX,float tanHalfY,ScreenPoint& out){
    if(!Finite(point)||!Finite(eye)||!std::isfinite(tanHalfX)||!std::isfinite(tanHalfY)||tanHalfX<=0||tanHalfY<=0)return false;
    V f,r,u;if(!Unit(forward,f)||!Unit(right,r)||!Unit(up,u))return false;
    if(std::fabs(Dot(f,r))>1e-3f||std::fabs(Dot(f,u))>1e-3f||std::fabs(Dot(r,u))>1e-3f)return false;
    const V d=point-eye;const float z=Dot(d,f);if(!std::isfinite(z)||z<=1e-5f)return false;
    ScreenPoint p;p.depth=z;p.u=.5f+.5f*Dot(d,r)/(z*tanHalfX);p.v=.5f-.5f*Dot(d,u)/(z*tanHalfY);
    if(!std::isfinite(p.u)||!std::isfinite(p.v))return false;out=p;return true;
}
inline float CanvasToEye(float value,float inset){return .5f+inset*(value-.5f);}
inline bool CanvasPixel(float u,float v,unsigned width,unsigned height,int& x,int& y){
    if(!std::isfinite(u)||!std::isfinite(v)||u<0||u>1||v<0||v>1||width<2||height<2||width>10000000||height>10000000)return false;
    x=(int)std::lround(u*(width-1));y=(int)std::lround(v*(height-1));return true;
}
// u/v address the ORIGINAL GFx logical canvas, before inset/stereo remap.
// eyeU/eyeV address its visible inset on one eye for a matching reticle.
// Out-of-panel rays fail rather than clamp onto an unintended menu item.
inline bool Intersect(V origin,V direction,const Plane& p,unsigned canvasWidth,unsigned canvasHeight,Hit& out){
    if(!Finite(origin)||!Finite(p.centre)||!Finite(p.right)||!Finite(p.up)||!Finite(p.normal)||!std::isfinite(p.halfWidth)||!std::isfinite(p.halfHeight)||p.halfWidth<=0||p.halfHeight<=0||!std::isfinite(p.inset)||p.inset<=0||p.inset>1||canvasWidth<2||canvasHeight<2||canvasWidth>10000000||canvasHeight>10000000)return false;
    if(std::fabs(Dot(p.normal,p.normal)-1)>1e-3f||std::fabs(Dot(p.right,p.right)-1)>1e-3f||std::fabs(Dot(p.up,p.up)-1)>1e-3f||std::fabs(Dot(p.normal,p.right))>1e-3f||std::fabs(Dot(p.normal,p.up))>1e-3f||std::fabs(Dot(p.right,p.up))>1e-3f)return false;
    V dir;if(!Unit(direction,dir))return false;
    const float denominator=Dot(dir,p.normal);if(!std::isfinite(denominator)||std::fabs(denominator)<1e-5f)return false;
    const float t=Dot(p.centre-origin,p.normal)/denominator;if(!std::isfinite(t)||t<=1e-5f)return false;
    Hit h;h.world=origin+dir*t;h.distance=t;V relative=h.world-p.centre;
    const float x=Dot(relative,p.right)/p.halfWidth,y=Dot(relative,p.up)/p.halfHeight;
    h.eyeU=.5f+.5f*x;h.eyeV=.5f-.5f*y;h.u=.5f+.5f*x/p.inset;h.v=.5f-.5f*y/p.inset;
    if(!Finite(h.world)||!std::isfinite(h.u)||!std::isfinite(h.v)||h.u<-.000001f||h.u>1.000001f||h.v<-.000001f||h.v>1.000001f)return false;
    h.u=std::max(0.f,std::min(1.f,h.u));h.v=std::max(0.f,std::min(1.f,h.v));
    if(!CanvasPixel(h.u,h.v,canvasWidth,canvasHeight,h.pixelX,h.pixelY))return false;out=h;return true;
}
enum class Context { Gameplay, Menu };
struct TriggerResult { bool selectPressed=false,fireHeld=false; };
// One instance per physical trigger. Run ONCE on action update, never per eye.
// Menu click is an edge; caller sends a paired down/up. Holding across any
// context/focus boundary stays suppressed until a physical trigger release.
struct TriggerGate {
    bool initialized=false,blocked=true,previousHeld=false;Context previousContext=Context::Gameplay;
    TriggerResult Update(Context context,bool inputAvailable,bool held,bool pointerHit){
        TriggerResult r;
        if(!initialized||context!=previousContext){initialized=true;blocked=held;previousHeld=held;previousContext=context;}
        if(!inputAvailable){blocked=true;previousHeld=held;return r;}
        if(!held){blocked=false;previousHeld=false;return r;}
        if(blocked){previousHeld=true;return r;}
        const bool edge=!previousHeld;previousHeld=true;
        r.selectPressed=context==Context::Menu&&edge&&pointerHit;
        r.fireHeld=context==Context::Gameplay;return r;
    }
};
}
