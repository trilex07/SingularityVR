#pragma once

struct RecenterChord {
    bool armed = true;
    bool leftSeen = false;
    bool rightSeen = false;
    unsigned leftTick = 0;
    unsigned rightTick = 0;
    unsigned tick = 0;
    unsigned window = 24; // ~200 ms at 120 Hz; the caller may replace it.

    bool Update(bool left, bool right) {
        ++tick;
        if (!left && !right) { armed = true; leftSeen = rightSeen = false; return false; }
        if (left && !leftSeen) { leftSeen = true; leftTick = tick; }
        if (right && !rightSeen) { rightSeen = true; rightTick = tick; }
        if (!armed || !leftSeen || !rightSeen) return false;
        const unsigned d = leftTick > rightTick ? leftTick - rightTick : rightTick - leftTick;
        if (d <= window) { armed = false; return true; }
        if (!left || !right) { leftSeen = left; rightSeen = right; }
        return false;
    }
};
