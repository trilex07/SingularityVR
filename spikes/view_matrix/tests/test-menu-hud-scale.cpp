#include "../menu_hud_scale.h"
#include "../ui_pointer.h"
#include <initializer_list>
#include <cmath>
#include <cstdio>

static int g_fails = 0;

static void Expect(bool cond, const char* name) {
    if (cond) {
        std::printf("PASS %s\n", name);
        return;
    }
    std::printf("FAIL %s\n", name);
    ++g_fails;
}

static bool Near(float a, float b) {
    return std::fabs(a - b) < 0.0001f;
}

int main() {
    Expect(StationMovieClass("RvUIGFxMovie_WeaponLocker"),"exact locker RTT owner");
    Expect(StationMovieClass("RvUIGFxMovie_BarisovStation") && StationMovieClass("RvUIGFxMovie_UseStation"),"upgrade station RTT owners");
    Expect(!StationMovieClass(nullptr) && !StationMovieClass("RvUIGFxMovie_Notepad") &&
           !StationMovieClass("RvUIGFxMovie_Dialog") && !StationMovieClass("RvUIGFxMovie_WeaponLockerOther"),"notes/dialogs and lookalikes unchanged");
    Expect(Near(StationCapturedInset(true,false,.84f,50),.42f),"captured station gets configured half-size");
    Expect(Near(StationCapturedInset(false,false,.84f,50),.84f) &&
           Near(StationCapturedInset(true,true,.84f,50),.84f),"nonstation and mod panel preserve size");
    // Captured INI is 100. The old shared clamp produced 0.84 for class 1.
    Expect(Near(ClassBackbufferScale(1, true, 100), 0.45f),
           "class 1 with MenuInsetPct 100 uses the 0.45 safe area");
    Expect(Near(ClassBackbufferScale(1, true, 70), 0.45f),
           "class 1 at source default is compacted to 0.45");
    Expect(Near(ClassBackbufferScale(1, true, 40), 0.40f),
           "class 1 keeps an INI that is already tighter than 70");
    Expect(Near(ClassBackbufferScale(1, false, 100), 0.45f),
           "class 1 cap does not depend on the menuSafe flag");

    Expect(Near(ClassBackbufferScale(2, true, 100), 0.84f),
           "class 2 keeps the 0.84 limit");
    Expect(Near(ClassBackbufferScale(3, true, 100), 0.84f),
           "class 3 keeps the 0.84 limit");
    Expect(Near(ClassBackbufferScale(4, true, 100), 0.84f),
           "class 4 menuSafe clamp is unchanged");
    Expect(Near(ClassBackbufferScale(2, true, 60), 0.60f),
           "class 2 keeps a tighter INI");
    Expect(Near(ClassBackbufferScale(0, false, 100), 1.00f),
           "a non-menu backbuffer draw is not clamped");
    Expect(Near(ClassBackbufferScale(2, false, 100), 1.00f),
           "class 2 without menuSafe is not given the class-1 cap");
    Expect(Near(MenuPlacementClipY(2), -.16f), "main menu shifts 8 percent down");
    for(int cls:{0,1,3,4}) Expect(Near(MenuPlacementClipY(cls),0.f), "other menus have no vertical shift");
    Expect(Near(StationMenuInset(4, false, 0.84f, 50), 0.84f * 0.50f),
           "class 4 station inset scales from menuSafe baseline");
    Expect(Near(StationMenuClipY(4), -0.12f), "class 4 station vertical centering nudge");
    using namespace uipointer;
    VrMenuPanel main;
    HeadPlane({0,0,0},{0,0,-1},{1,0,0},{0,1,0},1.2f,1.3f,1.33f,.84f,main.plane);
    main.plane.centre=main.plane.centre+main.plane.up*(MenuPlacementClipY(2)*main.plane.halfHeight);
    ScreenPoint screen;
    Expect(ProjectPoint(main.plane.centre,{0,0,0},{0,0,-1},{1,0,0},{0,1,0},1.3f,1.33f,screen)
           && Near(screen.v,.58f), "main plane centre projects 8 percent below centre");
    for(unsigned k=0;k<4;++k) {
        Hit hit; const V point=main.Corner(k);
        Expect(main.Ray({0,0,0},point,hit) && Near(hit.u,(k&1)?1.f:0.f)
               && Near(hit.v,(k&2)?1.f:0.f), "translated menu corners and laser agree");
    }
    for(int cls:{0,1,3,4}){
        Expect(Near(FrontMenuInset(cls,false,.84f,88),.84f),"other menu classes keep accepted size");
        Expect(Near(FrontMenuDistance(cls,false,950,1400),.95f),"other menu classes keep accepted depth");
    }
    Expect(Near(FrontMenuInset(2,true,.84f,88),.84f)&&Near(FrontMenuDistance(2,true,950,1400),.95f),"VR mod panel does not inherit frontend tuning");
    Expect(Near(FrontMenuInset(2,false,.84f,100),.84f)&&Near(FrontMenuDistance(2,false,950,0),.95f),"frontend exact opt-out");
    Expect(Near(FrontMenuInset(2,false,.84f,88),.7392f),"frontend is 12 percent smaller");
    Expect(Near(FrontMenuInset(2,false,.84f,-10),.588f)&&Near(FrontMenuInset(2,false,.84f,500),.84f),"scale bounds");
    Expect(Near(FrontMenuDistance(2,false,950,1),.6f)&&Near(FrontMenuDistance(2,false,950,4000),3.f),"distance bounds");
    const float scale=FrontMenuInset(2,false,.84f,88),distance=FrontMenuDistance(2,false,950,1400);
    HeadPlane({0,0,0},{0,0,-1},{1,0,0},{0,1,0},distance,1.3f,1.33f,scale,main.plane);
    main.plane.centre=main.plane.centre+main.plane.up*(MenuPlacementClipY(2)*main.plane.halfHeight);
    for(unsigned k=0;k<4;++k){
        Hit hit;const auto point=main.Corner(k);
        Expect(main.Ray({.15f,-.1f,.1f},point-V{.15f,-.1f,.1f},hit)&&Near(hit.u,(k&1)?1.f:0.f)&&Near(hit.v,(k&2)?1.f:0.f),"new presentation corner matches controller canvas UV");
        Expect(ProjectPoint(point,{0,0,0},{0,0,-1},{1,0,0},{0,1,0},1.3f,1.33f,screen)&&
            Near(screen.u,.5f+((k&1)?.5f:-.5f)*scale)&&Near(screen.v,.58f+((k&2)?.5f:-.5f)*scale),"new menu projection matches native backbuffer fallback scale/offset");
    }
    return g_fails == 0 ? 0 : 1;
}
