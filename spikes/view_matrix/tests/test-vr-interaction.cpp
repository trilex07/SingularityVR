#include "../ui_pointer.h"
#include "../support_grip.h"
#include "../shadow_policy.h"
#include "../grip_latch.h"
#include "../recenter_chord.h"
#include <cassert>
#include <cmath>
#include <cstdio>

using namespace uipointer;
static bool close(float a,float b,float e=2e-5f){return std::fabs(a-b)<e;}
static V yaw(V v,float a){return {v.x*std::cos(a)-v.z*std::sin(a),v.y,v.x*std::sin(a)+v.z*std::cos(a)};}

static void menu_projection_tests(){
    const unsigned sizes[][2]={{5264,2684},{2632,2684},{1920,1080},{1280,1024}};
    int samples=0;
    for(const auto& size:sizes)for(int deg=-180;deg<180;deg+=15){
        const float a=deg*3.1415926535f/180.f;
        const V f=yaw({0,0,-1},a),r=yaw({1,0,0},a),up{0,1,0},head{1.7f,1.6f,-.8f};
        Plane plane;const float tanX=1.04f,tanY=1.06f,inset=.70f,distance=1.2f;
        assert(HeadPlane(head,f,r,up,distance,tanX,tanY,inset,plane));
        for(float u: {0.f,.2f,.5f,.8f,1.f})for(float v: {0.f,.25f,.5f,.75f,1.f}){
            const V target=plane.centre+r*((u-.5f)*2*plane.halfWidth*inset)+up*((.5f-v)*2*plane.halfHeight*inset);
            Hit hit;assert(Intersect(head,target-head,plane,size[0],size[1],hit));
            assert(close(hit.u,u)&&close(hit.v,v));
            int px=0,py=0;assert(CanvasPixel(hit.u,hit.v,size[0],size[1],px,py));
            assert(px==hit.pixelX&&py==hit.pixelY);
            for(int eye=0;eye<2;++eye){
                const float eyeSide=eye?1.f:-1.f;const V eyePos=head+r*(eyeSide*.032f);
                ScreenPoint centre,point;assert(ProjectPoint(plane.centre,eyePos,f,r,up,tanX,tanY,centre));
                assert(ProjectPoint(hit.world,eyePos,f,r,up,tanX,tanY,point));
                // This is the exact transform applied to the GFx draw before SBS remap.
                assert(close(point.u,centre.u+inset*(u-.5f)));
                assert(close(point.v,CanvasToEye(v,inset)));
                const float sbs=(eye+point.u)*(size[0]*.5f);
                assert(std::isfinite(sbs)&&sbs>=eye*(size[0]*.5f)-1&&sbs<=(eye+1)*(size[0]*.5f)+1);
            }
            ++samples;
        }
        Hit miss;assert(!Intersect(head,r,plane,size[0],size[1],miss));
        assert(!Intersect(head,f*(-1.f),plane,size[0],size[1],miss));
        ScreenPoint behind;assert(!ProjectPoint(head-f,head,f,r,up,tanX,tanY,behind));
    }
    CanvasMap native;
    int px=0,py=0;
    assert(MapCanvas(.5f,.0f,5264,2684,native,px,py)&&py==0);
    assert(MapCanvas(.5f,1.f,5264,2684,native,px,py)&&py==2683);
    native.flipY=false; assert(MapCanvas(.25f,.75f,1000,500,native,px,py)&&px==250&&py==374);
    TriggerGate gate;using C=Context;
    assert(!gate.Update(C::Gameplay,true,true,false).fireHeld);gate.Update(C::Gameplay,true,false,false);
    assert(gate.Update(C::Gameplay,true,true,false).fireHeld);
    assert(!gate.Update(C::Menu,true,true,true).selectPressed);gate.Update(C::Menu,true,false,true);
    assert(gate.Update(C::Menu,true,true,true).selectPressed);assert(!gate.Update(C::Menu,true,true,true).selectPressed);
    assert(!gate.Update(C::Gameplay,true,true,false).fireHeld);gate.Update(C::Gameplay,true,false,false);
    assert(gate.Update(C::Gameplay,true,true,false).fireHeld);
    std::printf("menu: %d ray/render/click samples; SBS, four sizes, 24 head rotations, trigger transitions\n",samples);
}

static void support_tests(){
    const long pi=5799;assert(SupportGripFor(pi).mode==SupportGripMode::AuthoredPair);
    const float angles[]={0.f,.78539816f,-.78539816f,1.5707963f,-1.5707963f,3.1415926f};
    int samples=0;
    for(float a:angles){
        float p[4][4]={{std::cos(a),std::sin(a),0,0},{-std::sin(a),std::cos(a),0,0},{0,0,1,0},{13,-7,4,1}},o[4][4]{};
        assert(ApplySupportGripTarget(o,p,pi));
        for(int i=0;i<4;++i)for(int j=0;j<4;++j)assert(close(o[i][j],p[i][j]));
        for(long weapon: {8297L,11387L,11389L}){
            assert(ApplySupportGripTarget(o,p,weapon));const auto s=SupportGripFor(weapon);
            for(int j=0;j<3;++j){const float expected=s.fwd*p[0][j]+s.right*p[1][j]+s.up*p[2][j];assert(close(o[3][j]-p[3][j],expected));}
            ++samples;
        }
    }
    std::printf("support: authored pistol delta identity; %d invariant long-gun rotation samples\n",samples);
}

static void shadow_tests(){
    assert(IsProjectedShadowMap(114,512,512));
    assert(!IsProjectedShadowMap(114,256,256));assert(!IsProjectedShadowMap(114,512,1024));
    assert(!IsProjectedShadowMap(21,512,512));
    assert(IsProjectedShadowRenderTarget(21,512,512,1));
    assert(!IsProjectedShadowRenderTarget(21,512,512,0));
    assert(!IsProjectedShadowRenderTarget(21,1024,1024,1));
    assert(!SuppressVrShadow(false,false,true));assert(!SuppressVrShadow(true,true,true));
    std::puts("shadow: exact sampled 512x512 R32F projection classification");
    assert(SuppressFirstPersonShadowCaster(true,true,false,5799,5799,3314));
    assert(SuppressFirstPersonShadowCaster(true,true,false,3314,5799,3314));
    assert(!SuppressFirstPersonShadowCaster(true,true,false,4000,5799,3314));
    assert(!SuppressFirstPersonShadowCaster(true,false,false,5799,5799,3314));
}

static void latch_tests(){
    griplatch::GripLatch s;
    s.Update(false,0,true,1,0);assert(!s.Latched());
    s.Update(true,0,true,1,1);assert(s.Latched());
    s.Update(true,100,true,1,2);assert(s.Latched());
    s.Update(false,0,true,1,3);assert(!s.Latched());
    s.Update(true,100,false,1,4);assert(!s.Latched());
    s.Update(true,100,true,1,5);assert(!s.Latched());
    s.Update(true,0,true,1,6);assert(s.Latched());
    s.Update(true,0,false,1,7);assert(s.Latched());
    s.Update(false,0,false,1,8);assert(!s.Latched());
    RecenterChord c; c.window=24;
    assert(!c.Update(true,false)); assert(!c.Update(false,false));
    assert(!c.Update(false,true)); assert(!c.Update(false,false));
    RecenterChord d; d.window=24;
    assert(!d.Update(true,false)); assert(d.Update(true,true));
    assert(!d.Update(true,true)); assert(!d.Update(false,false));
    std::puts("support latch: acquire, drift hold, release, tracking grace; recenter chord edges");
}

int main(){menu_projection_tests();support_tests();shadow_tests();latch_tests();std::puts("PASS");}
