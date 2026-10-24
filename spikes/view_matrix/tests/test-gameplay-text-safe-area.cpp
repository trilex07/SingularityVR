#include "../gameplay_text_safe_area.h"
#include <cassert>
#include <cmath>
#include <cstdio>

int main() {
    using namespace gameplaytextsafe;
    Placement p{};
    assert(Place(256.f, 256.f, p));
    assert(std::fabs(p.x-521.f)<.001f && std::fabs(p.y-439.f)<.001f);
    // Character 414's authored shape bounds are x=-9..247/y=-87..169.
    // The resulting content is inside x=390..900/y=350..610 for both eyes.
    assert(p.x-9>=390 && p.x+247<=900 && p.y-87>=350 && p.y+169<=610);
    const float eyeWidth=5264.f/2.f;
    for (int eye=0;eye<2;++eye) {
        const float left=eye*eyeWidth+(p.x-9.f)/1280.f*eyeWidth;
        const float right=eye*eyeWidth+(p.x+247.f)/1280.f*eyeWidth;
        assert(left>eye*eyeWidth && right<(eye+1)*eyeWidth);
    }
    assert(!Place(0.f, 256.f, p)); // unknown GFx path
    assert(!Place(256.f, 0.f, p));
    assert(!Place(800.f, 256.f, p)); // unrelated whole-HUD clip
    assert(!Place(256.f, 500.f, p));
    assert(!Place(NAN, 256.f, p));
    assert(AuthoredPosition(20.f,600.f));
    assert(!AuthoredPosition(5000.f,600.f));
    std::puts("gameplay location text safe area: targeted geometry and both-eye bounds PASS");
}
