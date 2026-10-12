#pragma once
// Per-hand IK continuity. Section 0 (gloves) and section 1 (arms) can both reach
// PrepareIndependentLeft in one frame. g_armPaletteFrameToken is that frame's id:
// the second section reuses the first solve instead of advancing the elbow again.
#include "arm_natural.h"
#include <cstring>

namespace armiksession {

enum class Step { Disabled, Lost, Reuse, Incompatible, Solved, Failed };

struct HandSlot {
    armik::State state{};
    armbind::Deltas deltas{};
    float meshDelta[4][4]{};
    unsigned frame = ~0u;
    unsigned advances = 0;
    bool haveResult = false;
};

struct Session {
    HandSlot hand[2]{}; // 0 = physical left, 1 = physical right
    int mode = 0;
    bool modeSeen = false;
};

inline void InvalidateHand(Session& s, int side) {
    if (side < 0 || side > 1) return;
    s.hand[side] = {};
    s.hand[side].frame = ~0u;
}

inline void Reset(Session& s) {
    const int mode = s.mode;
    const bool seen = s.modeSeen;
    s = {};
    s.mode = mode;
    s.modeSeen = seen;
    s.hand[0].frame = ~0u;
    s.hand[1].frame = ~0u;
}

// Returns true when the mode value changed and both hand states were dropped.
inline bool NoteMode(Session& s, int mode) {
    if (!s.modeSeen) {
        s.modeSeen = true;
        s.mode = mode;
        return false;
    }
    if (s.mode == mode) return false;
    s.mode = mode;
    Reset(s);
    return true;
}

inline bool MeshDeltaCompatible(const float a[4][4], const float b[4][4]) {
    if (!a || !b) return false;
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            if (!std::isfinite(a[r][c]) || !std::isfinite(b[r][c]) || std::fabs(a[r][c] - b[r][c]) > 1e-4f)
                return false;
    return true;
}

inline Step Consider(Session& s, int side, bool handValid, unsigned frameToken, int mode,
                     const float meshDelta[4][4], armbind::Deltas& out) {
    NoteMode(s, mode);
    if (mode != 1) return Step::Disabled;
    if (side < 0 || side > 1) return Step::Failed;
    if (!handValid) {
        InvalidateHand(s, side);
        return Step::Lost;
    }
    HandSlot& h = s.hand[side];
    if (h.haveResult && h.frame == frameToken) {
        if (!MeshDeltaCompatible(h.meshDelta, meshDelta)) return Step::Incompatible;
        out = h.deltas;
        return Step::Reuse;
    }
    armik::State next = h.state;
    armbind::Deltas built{};
    if (!armnatural::Build(side == 1, meshDelta, next, built)) return Step::Failed;
    h.state = next;
    h.deltas = built;
    if (meshDelta) std::memcpy(h.meshDelta, meshDelta, sizeof(h.meshDelta));
    h.frame = frameToken;
    h.haveResult = true;
    h.advances += 1;
    out = built;
    return Step::Solved;
}

}  // namespace armiksession
