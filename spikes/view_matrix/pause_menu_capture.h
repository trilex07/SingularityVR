#pragma once
#include "menu_capture_owner.h"
#include <cstdint>

// Class 3 (RvUIGFxMovie_Pause) renders into a scene-sized private colour
// target. Hook_SetRenderTarget only calls CaptureVrMenuTarget when the target
// is not scene-sized, so the focus-token change releases the previous texture
// and nothing replaces it. This decision is the class-3 exception.
//
// Class 2 is Ignore: the 1280x720 main-menu provider is unchanged.
// A hold whose token does not match the offer is Reject, so a texture captured
// for an earlier movie cannot be provided after the token changes.
// Keep means this pause generation already owns a surface; a later target must
// not replace it.
namespace pausecapture {

enum class Decision {
    Ignore = 0,
    Reject = 1,
    Keep = 2,
    Accept = 3
};

struct Offer {
    int menuClass = 0;
    bool backbuffer = false;
    bool colour = false;
    int width = 0;
    int height = 0;
    std::uintptr_t surface = 0;
    std::uint64_t token = 0;
};

inline Decision Decide(const Offer& offer, const menucapture::Owner& owner) {
    if (offer.menuClass != 3) return Decision::Ignore;
    if ((offer.token & 0xffffffffu) != 3u) return Decision::Reject;
    if (owner.state != offer.token) return Decision::Reject;
    if (owner.CanProvide(offer.token)) return Decision::Keep;
    if (offer.backbuffer || !offer.colour || offer.surface == 0) return Decision::Reject;
    if (offer.width < 640 || offer.height < 360) return Decision::Reject;
    const float aspect = (float)offer.width / (float)offer.height;
    // 1280x720 is 1.78. The live scene-sized pause target (5264x2684) is 1.96.
    // The class-2 gate stops at 2048x1536, which is why that target never registered.
    if (aspect <= 1.20f || aspect >= 2.40f) return Decision::Reject;
    return Decision::Accept;
}

}
