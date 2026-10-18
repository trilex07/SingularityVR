#include <cassert>
#include <cmath>
#include <cstdint>
#include <utility>
#include <array>
#include "../hand_hud_policy.h"
#include "../menu_pointer_design.h"
#include "../modal_menu_policy.h"
#include "../ui_pointer.h"

static bool Near(float a,float b,float epsilon=1e-5f){return std::fabs(a-b)<=epsilon;}
static std::array<float,4> Transform(const float* m,float x,float y){
    return {x*m[0]+y*m[4]+m[12],x*m[1]+y*m[5]+m[13],
            x*m[2]+y*m[6]+m[14],x*m[3]+y*m[7]+m[15]};
}

static void TestDirectCanvasProjection(){
    // Real GFx convention: native pixel coordinates first become clip XY.
    const float source[16]={.002f,0,0,0,0,-.003f,0,0,0,0,1,0,-.7f,.5f,0,1};
    for(float inset:{.84f,.4f})for(float yaw:{0.f,.65f}){
        const float sy=std::sin(yaw*.5f),cy=std::cos(yaw*.5f);
        auto rotate=[&](uipointer::V v){return uipointer::Rotate(0,sy,0,cy,v);};
        uipointer::Plane plane;
        assert(uipointer::HeadPlane({.2f,-.1f,.3f},rotate({0,0,-1}),rotate({1,0,0}),
            rotate({0,1,0}),.95f,1.3f,1.1f,inset,plane));
        for(int eye=0;eye<2;++eye){
            const auto eyePos=uipointer::V{.2f,-.1f,.3f}+rotate({eye?.035f:-.035f,eye?.003f:0.f,eye?.002f:0.f});
            // Also exercise a small eye rotation; centre-only offsets cannot
            // reproduce this projection away from the panel centre.
            const float eyeYaw=yaw+(eye?.02f:0.f);
            auto eyeRotate=[&](uipointer::V v){return uipointer::Rotate(0,std::sin(eyeYaw*.5f),0,std::cos(eyeYaw*.5f),v);};
            float out[16];
            assert(uipointer::ProjectCanvasMatrix(plane,eyePos,eyeRotate({0,0,-1}),
                eyeRotate({1,0,0}),eyeRotate({0,1,0}),1.3f,1.1f,source,out));
            for(float u:{0.f,.23f,.5f,.91f,1.f})for(float v:{0.f,.31f,.5f,1.f}){
                const float x=(2*u-1-source[12])/source[0],y=(1-2*v-source[13])/source[5];
                const auto clip=Transform(out,x,y);assert(clip[3]>0);
                uipointer::ScreenPoint expected;
                assert(uipointer::ProjectPoint(uipointer::PointAtCanvasUv(plane,u,v),eyePos,
                    eyeRotate({0,0,-1}),eyeRotate({1,0,0}),eyeRotate({0,1,0}),1.3f,1.1f,expected));
                assert(Near(.5f+.5f*clip[0]/clip[3],expected.u));
                assert(Near(.5f-.5f*clip[1]/clip[3],expected.v));
            }
            float sentinel[16];for(auto& value:sentinel)value=123.f;
            assert(!uipointer::ProjectCanvasMatrix(plane,eyePos,eyeRotate({0,0,-1}),
                eyeRotate({1,0,0}),eyeRotate({0,1,0}),0,1.1f,source,sentinel));
            for(auto value:sentinel)assert(value==123.f);
        }
    }
}

int main(){
    TestDirectCanvasProjection();
    for(bool flip:{false,true})for(float u:{0.f,.13f,.5f,.91f,1.f})for(float v:{0.f,.2f,.7f,1.f}){
        uipointer::CanvasMap crop{.1f,.2f,.9f,.85f,flip};
        int x=0,y=0;float actualU=0,actualV=0;
        assert(uipointer::MapCanvas(u,v,5904,3012,crop,x,y));
        assert(uipointer::UnmapCanvas(x,y,5904,3012,crop,actualU,actualV));
        assert(std::fabs(actualU-u)<.0003f&&std::fabs(actualV-v)<.0003f);
    }
    float badU=0,badV=0;
    assert(!uipointer::UnmapCanvas(-1,0,5904,3012,{},badU,badV));
    assert(!uipointer::UnmapCanvas(0,0,1,3012,{},badU,badV));
    assert(handhud::Visible(true,true,true,true,false,false,false,false));
    assert(!handhud::Visible(true,true,true,false,false,false,false,true));
    assert(!handhud::Visible(true,true,true,true,true,false,false,false));
    assert(!handhud::Visible(true,true,true,true,false,true,false,false));
    assert(!handhud::Visible(true,true,true,true,false,false,true,false));
    assert(handhud::ScaledExtent(200)==176&&handhud::ScaledExtent(100)==88);
    assert((handhud::WithAlphaPct(0xFFFFFFFFu,handhud::kPlateAlphaPct)>>24)==209u);
    assert((handhud::WithAlphaPct(0xFFFFFFFFu,handhud::kDigitAlphaPct)>>24)==240u);

    assert(Near(menupointer::PulseScale(0),1.f));
    assert(Near(menupointer::PulseScale(350),1.08f));
    assert(Near(menupointer::PulseScale(1050),.92f));
    assert(Near(menupointer::PulseScale(1400),1.f));
    const auto open=menupointer::AtTime(350,false,2828.f);
    const auto pressed=menupointer::AtTime(350,true,2828.f);
    assert(open.outerRadius>pressed.outerRadius&&open.coreRadius>pressed.coreRadius);
    assert(open.outerRadius>open.coreRadius&&open.ringThickness>0);

    assert(modalmenu::IsSaveDialogTransition(true,true));
    assert(!modalmenu::IsSaveDialogTransition(false,true));
    assert(!modalmenu::IsSaveDialogTransition(true,false));
    assert(modalmenu::CaptureExactDialog(true,true,true));
    assert(!modalmenu::CaptureExactDialog(true,true,false));
    assert(!modalmenu::CaptureExactDialog(true,false,true));
    assert(modalmenu::DirectDialogCanSuppressParent(true,true,true,true));
    assert(!modalmenu::DirectDialogCanSuppressParent(true,true,false,true));
    assert(!modalmenu::DirectDialogCanSuppressParent(false,true,true,true));
    assert(!modalmenu::DirectDialogCanSuppressParent(true,true,true,false));
    assert(modalmenu::SuppressParentSlots(true,true,true,false));
    assert(!modalmenu::SuppressParentSlots(true,false,true,false));
    assert(!modalmenu::SuppressParentSlots(true,true,false,false));
    assert(!modalmenu::SuppressParentSlots(true,true,true,true));

    uipointer::Plane p;
    p.centre={0,0,-1};p.right={1,0,0};p.up={0,1,0};p.normal={0,0,1};
    p.halfWidth=.8f;p.halfHeight=.45f;p.inset=.72f;
    const auto centre=uipointer::PointAtCanvasUv(p,.5f,.5f);
    const auto tl=uipointer::PointAtCanvasUv(p,0,0);
    const auto br=uipointer::PointAtCanvasUv(p,1,1);
    assert(Near(centre.x,p.centre.x)&&Near(centre.y,p.centre.y)&&Near(centre.z,p.centre.z));
    assert(Near(tl.x,p.centre.x-p.halfWidth*p.inset));
    assert(Near(tl.y,p.centre.y+p.halfHeight*p.inset));
    assert(Near(br.x,p.centre.x+p.halfWidth*p.inset));
    assert(Near(br.y,p.centre.y-p.halfHeight*p.inset));
    for(const auto uv : {std::pair<float,float>{.12f,.2f},std::pair<float,float>{.5f,.5f},
                         std::pair<float,float>{.91f,.83f}}){
        const auto endpoint=uipointer::PointAtCanvasUv(p,uv.first,uv.second);
        uipointer::Hit hit;
        assert(uipointer::Intersect({0,0,0},endpoint,p,2,2,hit));
        assert(Near(hit.u,uv.first)&&Near(hit.v,uv.second));
    }
    return 0;
}
