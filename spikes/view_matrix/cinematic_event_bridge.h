#pragma once
#include <cstdint>

namespace cineevent {
// Inputs must come from the validated local RvPlayerController event, not a
// name-only timer or an arbitrary OnToggle sequence object. No native writes.
inline bool Accept(bool enabled, bool eventIdentity, bool localOwner,
                   bool reflectedLayout, bool readable, const std::uint32_t* values) {
    if(!enabled || !eventIdentity || !localOwner || !reflectedLayout || !readable || !values)
        return false;
    for(unsigned i=0;i<6;++i)if(values[i]>1)return false;
    return true; // Both native ON and native OFF are authoritative.
}
}
