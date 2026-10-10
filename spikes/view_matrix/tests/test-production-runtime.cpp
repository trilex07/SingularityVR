#include "../ui_pointer.h"
#include "../grip_latch.h"
#include <cassert>
#include <cmath>
#include <cstdio>

int main() {
    using namespace uipointer;
    unsigned samples=0;
    for(unsigned width:{1280u,1920u,2632u,5264u})
    for(unsigned height:{720u,1080u,2684u})
    for(int degrees=-180;degrees<180;degrees+=15) {
        const float a=degrees*3.14159265f/180.f;
        const V f{std::sin(a),0,-std::cos(a)},r{std::cos(a),0,std::sin(a)},up{0,1,0};
        const V head{2,1.6f,-3};
        VrMenuPanel p;
        assert(HeadPlane(head,f,r,up,.95f,1.04f,1.06f,.7f,p.plane));
        for(float v:{0.f,.25f,.5f,.75f,1.f})
        for(float u:{0.f,.25f,.5f,.75f,1.f}) {
            // Rendering and hit testing consume the exact same production panel.
            const V top=p.Corner(0)*(1-u)+p.Corner(1)*u;
            const V bottom=p.Corner(2)*(1-u)+p.Corner(3)*u;
            const V world=top*(1-v)+bottom*v;
            Hit hit; assert(p.Ray(head,world-head,hit));
            assert(std::fabs(hit.u-u)<1e-5f && std::fabs(hit.v-v)<1e-5f);
            int x,y; assert(MapCanvas(hit.u,hit.v,width,height,p.canvas,x,y));
            assert(std::abs(x-(int)std::lround(u*(width-1)))<=1);
            assert(std::abs(y-(int)std::lround(v*(height-1)))<=1);
            ScreenPoint screen;assert(ProjectPoint(world,head,f,r,up,1.04f,1.06f,screen));
            assert(std::fabs(screen.v-CanvasToEye(v,.7f))<1e-5f);
            ++samples;
        }
    }
    griplatch::GripLatch latch;
    for(unsigned t=0;t<100;++t){latch.Update(false,0,true,1,t);assert(!latch.Latched());}
    latch.Update(true,0,true,1,101);assert(latch.Latched());
    latch.Update(true,100,true,1,102);assert(latch.Latched());
    latch.Update(false,0,true,1,103);assert(!latch.Latched());
    for(unsigned t=104;t<204;++t){latch.Update(false,0,true,1,t);assert(!latch.Latched());}
    latch.Update(true,0,true,1,205);assert(latch.Latched());
    latch.Update(true,0,false,1,206);assert(latch.Latched());
    latch.Update(false,0,false,1,207);assert(!latch.Latched());
    latch.Update(true,100,true,1,208);assert(!latch.Latched());
    latch.Update(true,0,true,1,209);assert(latch.Latched());
    latch.Update(true,0,true,2,210);assert(!latch.Latched());
    std::printf("PASS: %u production-panel samples; grip near/release/grace/weapon regression\n",samples);
}
