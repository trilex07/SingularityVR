#pragma once
#include <cmath>
inline bool TwoHandGripAllowed(const float delta[3], const float forward[3], bool held,
                              bool tracked, bool wasActive, bool pistol = false) {
    if (!held || !tracked) return false;
    const float distance = std::sqrt(delta[0]*delta[0]+delta[1]*delta[1]+delta[2]*delta[2]);
    // A pistol support hand is naturally beside the primary grip rather than in front of
    // the weapon. The old universal 12 cm minimum and forward alignment rejected the
    // exact close-in gesture used to acquire a pistol with the off-hand. Keep the stricter
    // rifle/shotgun gate unchanged; pistol acquisition is still explicit (Grip held) and
    // bounded to the hands' near zone. Once latched, GripLatch deliberately ignores distance
    // until release, so moving the support hand away cannot silently detach it.
    const float minDistance = pistol ? .045f : .12f;
    const float maxDistance = pistol ? (wasActive ? .55f : .38f)
                                     : (wasActive ? .65f : .50f);
    if (!std::isfinite(distance) || distance < minDistance || distance > maxDistance) return false;
    const float alignment=(delta[0]*forward[0]+delta[1]*forward[1]+delta[2]*forward[2])/distance;
    if (pistol) return alignment > -0.35f;
    return alignment > (wasActive ? .15f : .50f);
}
