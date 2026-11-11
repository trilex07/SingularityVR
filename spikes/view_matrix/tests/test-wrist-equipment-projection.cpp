#include "../ui_pointer.h"
#include "../wrist_equipment_policy.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <initializer_list>
using uipointer::V;
static V Panel(float x,float y,float yaw,float pitch,float roll,V wrist){
    const float cy=cosf(yaw),sy=sinf(yaw),cp=cosf(pitch),sp=sinf(pitch),
                cr=cosf(roll),sr=sinf(roll);
    // Synthetic tracked-wrist pose: yaw, pitch, roll applied to a physical
    // 15.5 x 10.4 cm panel. This is a projection fixture, not a native bind.
    V p{(x-.5f)*.155f,(.5f-y)*.104f,0};
    const V r{p.x*cr-p.y*sr,p.x*sr+p.y*cr,p.z};
    const V t{r.x,r.y*cp-r.z*sp,r.y*sp+r.z*cp};
    return {wrist.x+t.x*cy+t.z*sy,wrist.y+t.y,wrist.z-t.x*sy+t.z*cy};
}
int main(){
    const V forward{0,0,-1},right{1,0,0},up{0,1,0};
    const V eyeL{-.032f,0,0},eyeR{.032f,0,0};
    const V wrist{-.22f,-.12f,-.42f};
    for(float yaw:{0.f,.3f,-.3f})for(float pitch:{0.f,.2f,-.2f})
        for(float roll:{0.f,.4f,-.4f}){
            uipointer::ScreenPoint l,r;
            const V p=Panel(.5f,.5f,yaw,pitch,roll,wrist);
            assert(uipointer::ProjectPoint(p,eyeL,forward,right,up,1,1,l));
            assert(uipointer::ProjectPoint(p,eyeR,forward,right,up,1,1,r));
            assert(std::fabs((l.u-r.u)-(.064f/(.42f*2)))<1e-4f);
            assert(l.v==r.v);
            uipointer::ScreenPoint corner;
            assert(uipointer::ProjectPoint(Panel(1.f,0.f,yaw,pitch,roll,wrist),
                eyeL,forward,right,up,1,1,corner));
            assert(std::fabs(corner.u-l.u)+std::fabs(corner.v-l.v)>.01f);
        }
    assert(wristhud::Current(100,200));
    assert(!wristhud::Current(100,300));
    std::puts("PASS wrist panel stereo parallax, translation, yaw, pitch, roll, freshness");
}
