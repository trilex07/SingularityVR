#pragma once
#include "menu_capture_owner.h"
#include <cstdint>

// Pause registration gate.
//
// HudStereoPair reaches its class-3 draw only after the non-scene guard has
// already returned, so the current target is scene-classified. The saved
// headset trace for RvUIGFxMovie_Pause (frames 20-27 of that focus token)
// contains only 5264x2684 scene targets. The private GFx texture that the
// main menu registers is 1280x720 and is not scene-classified, and it does
// not appear in the pause sample. Size, colour format and class-3 focus do
// not identify a pause movie. identifiedPauseMovie must be set by a caller
// that has already proved the surface is that movie. Nothing in the runtime
// has that proof, so the live capture path is not registered.
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
    bool sceneClassified = false;
    bool identifiedPauseMovie = false;
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
    // The framebuffer, its SBS image, and any other scene-sized target.
    if (offer.sceneClassified || offer.backbuffer) return Decision::Reject;
    // Class, dimensions and format are not texture identity.
    if (!offer.identifiedPauseMovie || !offer.colour || offer.surface == 0) return Decision::Reject;
    if (offer.width < 640 || offer.height < 360 || offer.width > 2048 || offer.height > 1536)
        return Decision::Reject;
    const float aspect = (float)offer.width / (float)offer.height;
    if (aspect <= 1.20f || aspect >= 2.20f) return Decision::Reject;
    return Decision::Accept;
}

}
