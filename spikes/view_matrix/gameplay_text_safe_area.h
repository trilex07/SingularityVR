#pragma once
#include <cmath>

namespace gameplaytextsafe {
// The fingerprinted HUD movie's authored stage is 1280x720. Character 414
// contains three text fields and shape 218. The latter contributes local
// x=-9..247, y=-87..169 px after its authored matrix. A 256px holder thus
// needs origin (521,439) to centre its content near (640,480).
struct Placement { float x, y; };
inline bool Place(float width, float height, Placement& out) {
    if (!std::isfinite(width) || !std::isfinite(height) ||
        width < 200.f || width > 500.f || height < 100.f || height > 400.f)
        return false; // missing GFx path reads as zero; refuse an unknown clip
    out.x = 9.f + (1280.f - width) * .5f;
    out.y = 439.f;
    return out.x >= 399.f && out.x <= 549.f;
}
inline bool AuthoredPosition(float x, float y) {
    return std::isfinite(x) && std::isfinite(y) &&
           x >= -100.f && x <= 1280.f && y >= -100.f && y <= 720.f;
}
}
