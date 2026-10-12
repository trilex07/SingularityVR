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
    return g_fails == 0 ? 0 : 1;
}
