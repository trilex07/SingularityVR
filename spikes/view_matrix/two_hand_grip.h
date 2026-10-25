#pragma once
#include <cmath>
inline bool TwoHandGripAllowed(const float delta[3], const float forward[3], bool held,
                              bool tracked, bool wasActive, bool pistol = false,
                              bool closeLongGun = false) {
    if (!held || !tracked) return false;
    const float distance = std::sqrt(delta[0]*delta[0]+delta[1]*delta[1]+delta[2]*delta[2]);
    // A pistol support hand is naturally beside the primary grip rather than in front of
    // the weapon. The old universal 12 cm minimum and forward alignment rejected the
    // exact close-in gesture used to acquire a pistol with the off-hand. Keep the stricter
    // rifle/shotgun gate unchanged; pistol acquisition is still explicit (Grip held) and
    // bounded to the hands' near zone. Once latched, GripLatch deliberately ignores distance
    // until release, so moving the support hand away cannot silently detach it.
    // The physical Volk probe recorded valid Grip presses at 10.15 and 11.94 cm
    // that were refused solely by the ordinary 12 cm minimum. Keep this smaller
    // near-hand floor confined to the explicit ordinary-long-gun opt-in.
    const float minDistance = (pistol || closeLongGun) ? .045f : .12f;
    const float maxDistance = pistol ? (wasActive ? .55f : .38f)
                                     : (wasActive ? .65f : .50f);
    if (!std::isfinite(distance) || distance < minDistance || distance > maxDistance) return false;
    const float alignment=(delta[0]*forward[0]+delta[1]*forward[1]+delta[2]*forward[2])/distance;
    if (pistol) return alignment > -0.35f;
    // With the independently tracked TMD glove the raw controller's -Z cone
    // does not coincide with the visible ordinary long-gun fore-end. A short,
    // explicit near-hand zone accepts a physical supporting grip without
    // weakening the existing 50 cm forward cone or Seeker's separate route.
    // This is opt-in at the caller and still requires a live Grip + tracking.
    if (closeLongGun && !wasActive && distance <= .30f && alignment > -.75f)
        return true;
    return alignment > (wasActive ? .15f : .50f);
}
