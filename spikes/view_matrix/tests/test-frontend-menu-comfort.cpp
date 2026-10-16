#include "../frontend_menu_comfort.h"
#include "../menu_hud_scale.h"
#include "../menu_submit_policy.h"
#include "../vr_scene_mode.h"
#include "../menu_capture_owner.h"
#include "../ui_pointer.h"
#include <cassert>
#include <cstdio>
#include <cmath>
struct Pass {
    int fail=0,step=0,endCount=0,restoreCount=0,rollbackCount=0;
    int pixels=17,state=23;bool begun=false,rollbackFails=false;
    bool Op(){return ++step!=fail;}
    bool Capture(){return Op();}
    bool SavePixels(){return Op();}
    bool Setup(){state=99;return Op();}
    bool Clear(){pixels=0;return Op();}
    bool BeginScene(){const bool good=Op();begun=good;return good;}
    bool DrawEye(int eye){assert(begun);pixels+=eye+1;return Op();}
    bool EndScene(){assert(begun);begun=false;++endCount;return Op();}
    bool Restore(){++restoreCount;state=23;return Op();}
    bool RestorePixels(){++rollbackCount;pixels=17;return !rollbackFails;}
};
int main(){
    frontmenu::Signals s{true,true,true,true,true,false,false,true,true,true,false,2};
    assert(frontmenu::Eligible(s));
    assert(frontmenu::OwnsPresentation(true,2,true,true,false));
    assert(!frontmenu::OwnsPresentation(true,2,true,true,true));
    assert(!frontmenu::OwnsPresentation(false,2,true,false,false));
    for(int c:{0,1,3,4})assert(!frontmenu::OwnsPresentation(true,c,true,false,false));
    assert(frontmenu::CurrentReceipt(true,4,4)&&!frontmenu::CurrentReceipt(true,4,5)&&!frontmenu::CurrentReceipt(false,4,4));
    for(int c=-1;c<7;++c){s.menuClass=c;assert(frontmenu::Eligible(s)==(c==2));}s.menuClass=2;
    for(auto member:{&frontmenu::Signals::enabled,&frontmenu::Signals::vr,&frontmenu::Signals::duplicated,
        &frontmenu::Signals::focused,&frontmenu::Signals::provider,&frontmenu::Signals::sourceCurrent,
        &frontmenu::Signals::planeValid,&frontmenu::Signals::menuMode}){
        auto off=s;off.*member=false;assert(!frontmenu::Eligible(off));
    }
    Pass lost;lost.fail=4;lost.rollbackFails=true;
    assert(!frontmenu::Render(lost).pixelsRestored);
    for(auto member:{&frontmenu::Signals::panel,&frontmenu::Signals::modal,&frontmenu::Signals::video}){
        auto blocked=s;blocked.*member=true;assert(!frontmenu::Eligible(blocked));
    }
    for(int c=0;c<5;++c)for(bool legacy:{false,true})for(bool done:{false,true}){
        assert(frontmenu::UseLegacyCrop(legacy,done,c)==(legacy&&!(done&&c==2)));
    }
    Pass success;const auto complete=frontmenu::Render(success);
    assert(complete.composed&&complete.stateRestored&&success.state==23&&success.endCount==1&&success.rollbackCount==0);
    for(int fail=1;fail<=9;++fail){
        Pass p;p.fail=fail;const auto result=frontmenu::Render(p);
        assert(!result.composed&&!p.begun);
        assert(p.restoreCount==(fail==1?0:1));
        assert(p.pixels==17); // includes a clear/draw that fails AFTER writing pixels
        if(fail>=6)assert(p.endCount==1);
        if(fail==9)assert(!result.stateRestored);
    }
    // Exactly captured success, not the next live menu, decides slot crop policy.
    const bool captured=frontmenu::UseLegacyCrop(true,complete.composed,2);
    s.menuClass=3;s.menuMode=false;
    assert(!captured&&!frontmenu::Eligible(s));
    assert(frontmenu::UseLegacyCrop(true,false,2)); // failed/transition frame keeps fallback
    menucapture::Owner owner;owner.Invalidate(1);assert(!owner.CanProvide(1));
    owner.Capture(1,42);assert(owner.CanProvide(1));owner.Invalidate(2);assert(!owner.CanProvide(2));
    // Boot -> frontend -> gameplay -> pause -> resumed gameplay -> returned frontend.
    vrscene::Controller modes;
    modes.Advance({false,false,true,false,true,false});assert(modes.Current()==vrscene::Mode::MenuFlat);
    modes.Advance({false,false,false,false,false,false});assert(modes.Current()==vrscene::Mode::GameplayStereo);
    modes.Advance({false,false,true,true,false,false});assert(modes.Current()==vrscene::Mode::MenuFlat);
    modes.Advance({false,false,false,false,false,false});assert(modes.Current()==vrscene::Mode::GameplayStereo);
    modes.Advance({true,false,true,false,true,false});assert(modes.Current()==vrscene::Mode::MenuFlat);
    modes.Advance({true,true,true,false,true,false});assert(modes.Current()==vrscene::Mode::VideoFlat);
    using namespace uipointer;
    const float inset=FrontMenuInset(2,false,.84f,80),distance=FrontMenuDistance(2,false,1200,1800);
    assert(std::fabs(inset-.672f)<.0001f&&std::fabs(distance-1.8f)<.0001f);
    VrMenuPanel panel;assert(HeadPlane({0,0,0},{0,0,-1},{1,0,0},{0,1,0},distance,1.3f,1.33f,inset,panel.plane));
    panel.plane.centre=panel.plane.centre+panel.plane.up*(MenuPlacementClipY(2)*panel.plane.halfHeight);
    for(unsigned k=0;k<4;++k){
        const V origin{.12f,-.2f,.1f};Hit hit;
        assert(panel.Ray(origin,panel.Corner(k)-origin,hit));
        assert(std::fabs(hit.u-((k&1)?1.f:0.f))<.0001f&&std::fabs(hit.v-((k&2)?1.f:0.f))<.0001f);
    }
    ScreenPoint left,right;
    assert(ProjectPoint(panel.plane.centre,{-.032f,0,0},{0,0,-1},{1,0,0},{0,1,0},1.3f,1.33f,left));
    assert(ProjectPoint(panel.plane.centre,{.032f,0,0},{0,0,-1},{1,0,0},{0,1,0},1.3f,1.33f,right));
    assert(left.u>right.u&&std::fabs((left.u-right.u)-.064f/(2*1.8f*1.3f))<.0001f);
    for(int c:{0,1,3,4}){
        assert(FrontMenuInset(c,false,.84f,80)==.84f&&FrontMenuDistance(c,false,1200,1800)==1.2f);
    }
    std::puts("PASS frontend policy, 9 transactional fault stages, lifecycle and 1.8m/80% laser geometry (CPU only)");
}
