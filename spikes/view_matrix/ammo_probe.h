#pragma once
// MAIN: trigger-edge + budget gate for the ammo property probe (diagnostic,
// default OFF). Pure logic, L1-tested in test-ammo-probe.cpp. The bounded
// dumper itself lives in d3d9.cpp next to the fire census (input thread):
// baseline snapshot per weapon object, changed-offsets-only after, N pulls
// then silent. Diffing the dumps across spaced shots names the mag counter
// (decrements per shot). Reserve moves on pickup, not on fire, so it is
// identified by value (mag-size knowledge: 12/30/6/5), not by motion.
namespace ammoprobe {

enum { kSlots = 1024 };  // int32 words covered: [0, 0x1000) of the weapon object
enum { kPawnSlots = 512 }; // pawn baseline window: [0, 0x800), once per session
enum { kUnreadable = 0x7fffffff }; // sentinel for a failed Readable guard

inline bool ShouldDump(bool down, bool wasDown, int left, bool gunLatched,
                       unsigned long long wobj) {
    return down && !wasDown && left > 0 && gunLatched && wobj != 0;
}

// Shared analysis helper: count differing snapshot slots.
inline int DiffCount(const int* a, const int* b, int n) {
    int c = 0;
    for (int i = 0; i < n; ++i)
        if (a[i] != b[i]) ++c;
    return c;
}

} // namespace ammoprobe
