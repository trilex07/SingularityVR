#include <cassert>
#include <cstdio>
#include "../pause_menu_head_overlay.h"

int main() {
    using namespace pausehead;
    assert(!RedirectOrthoDraws(3, false, true, true, true, false));
    assert(!RedirectOrthoDraws(3, true, false, true, true, false));
    assert(!RedirectOrthoDraws(3, true, true, false, true, false));
    assert(!RedirectOrthoDraws(3, true, true, true, false, false));
    assert(!RedirectOrthoDraws(3, true, true, true, true, true));
    assert(!RedirectOrthoDraws(2, true, true, true, true, false));
    assert(RedirectOrthoDraws(3, true, true, true, true, false));
    assert(kPoolWidth == 1280 && kPoolHeight == 720);
    assert(IniEnabled(1) && !IniEnabled(0));
    std::puts("pause head overlay: class 3 redirect gated by PauseHeadMenu and overlay flags");
    return 0;
}
