#pragma once
#include <cstdint>

namespace lockerowned {

// True when WeaponLocker focus is already established for this movie/class pair.
inline bool SameLockerFocusSession(bool sessionLive, uintptr_t activeMovie, uintptr_t activeCls, uintptr_t movie,
                                   uintptr_t cls) {
    return sessionLive && movie != 0 && cls != 0 && activeMovie == movie && activeCls == cls;
}

inline bool LockerFocusTransition(bool sessionLive, uintptr_t activeMovie, uintptr_t activeCls, uintptr_t movie,
                                  uintptr_t cls) {
    return !SameLockerFocusSession(sessionLive, activeMovie, activeCls, movie, cls);
}

} // namespace lockerowned
