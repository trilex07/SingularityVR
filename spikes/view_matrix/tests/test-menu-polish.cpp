#include <cassert>
#include <cmath>
#include <cstdint>
#include <utility>
#include "../hand_hud_policy.h"
#include "../menu_pointer_design.h"
#include "../modal_menu_policy.h"
#include "../ui_pointer.h"

static bool Near(float a,float b,float epsilon=1e-5f){return std::fabs(a-b)<=epsilon;}

int main(){
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
