#pragma once
#include <cmath>
inline bool TwoHandGripAllowed(const float delta[3], const float forward[3], bool held,
                              bool tracked, bool wasActive) {
    if (!held || !tracked) return false;
    const float distance = std::sqrt(delta[0]*delta[0]+delta[1]*delta[1]+delta[2]*delta[2]);
    if (!std::isfinite(distance) || distance < .12f || distance > (wasActive ? .65f : .50f)) return false;
    const float alignment=(delta[0]*forward[0]+delta[1]*forward[1]+delta[2]*forward[2])/distance;
    return alignment > (wasActive ? .15f : .50f);
}
