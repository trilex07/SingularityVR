#pragma once
// One physical squeeze owns one action. Never acquire a power halfway through
// a support/reload squeeze, or acquire support halfway through a power squeeze.
namespace tmdgravity {
struct Grip {
    bool prior=false, power=false;
    bool Update(bool enabled, bool held, bool eligible, bool reserved) {
        if (!held) { prior=false; power=false; return false; }
        if (!prior) power=enabled && eligible && !reserved;
        prior=true;
        if (!enabled || !eligible || reserved) power=false;
        return power;
    }
};
}
