#pragma once
#include "skeletal_palette.h"
#include <cmath>
#include <cstring>

// Material 1 of ch_inview_tmd_mesh: the SVTG index set retains short sleeve
// faces skinned to these seven bones as well as the native TMD hardware.
// GPU matrices are three output-component rows. TransformBone postmultiplies
// each ORIGINAL skin matrix by the same row-vector rigid delta as hardware.
namespace tmdsleeve {
constexpr unsigned kMap[15] = {7,46,8,27,43,41,44,45,42,29,28,30,50,85,49};
constexpr unsigned kSleeveBones[7] = {8,46,41,42,43,44,45};
inline bool CommitAllowed(bool svtgBound, bool independentTracking, bool staged, int regs) {
    return svtgBound && independentTracking && staged && regs==45;
}

inline bool Stage(const unsigned* map, unsigned slots, const float original[45][4],
                  const float hardwareMoved[45][4], const float delta[4][4],
                  float staged[45][4]) {
    if (!map || slots != 15) return false;
    for (unsigned slot=0; slot<slots; ++slot) if (map[slot] != kMap[slot]) return false;
    std::memcpy(staged, hardwareMoved, slots*3*4*sizeof(float));
    for (unsigned bone : kSleeveBones) {
        unsigned slot=0;
        while (slot<slots && map[slot]!=bone) ++slot;
        if (slot==slots) return false;
        skeletal::TransformBone(&staged[slot*3], &original[slot*3], delta);
        for (unsigned reg=slot*3; reg<slot*3+3; ++reg)
            for (float value : staged[reg]) if (!std::isfinite(value)) return false;
    }
    return true;
}
} // namespace tmdsleeve
