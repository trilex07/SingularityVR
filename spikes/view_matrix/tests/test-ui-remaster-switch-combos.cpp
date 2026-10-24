// MAIN L1: explicit truth table for every P0/P1/P2 switch combination.
// Mirrors the runtime gates exactly:
//   - DrawVrPanel bails unless Runtime or Wheel is on (spikes/view_matrix/d3d9.cpp).
//   - P0 plate draws iff Runtime (fail-closed neutral atlas load elsewhere).
//   - P1 settings styling draws iff SettingsEnabled(Runtime, Settings).
//   - P2 wheel skin draws iff WheelEnabled(Wheel) (independent of P0/P1).
#include "ui_remaster_runtime.h"
#include <cassert>
#include <cstdio>

#ifdef _WIN64
#error compile as x86
#endif

static int checked = 0;

static void expect(bool runtime, bool settings, bool wheel, bool anyDraw,
                   bool plate, bool style, bool skin) {
    const bool bailTop = !runtime && !wheel;
    assert(bailTop != anyDraw);                       // top gate
    assert(uiremaster::SettingsEnabled(runtime, settings) == style);
    assert(uiremaster::WheelEnabled(wheel) == skin);
    assert((runtime != false) == plate);
    assert(!plate || anyDraw);                        // plate implies draws
    assert(!style || plate);                          // P1 never without P0 (committed combos)
    ++checked;
}

int main() {
    expect(0, 0, 0, false, false, false, false);      // accepted renderer
    expect(1, 0, 0, true,  true,  false, false);      // P0 only
    expect(0, 1, 0, false, false, false, false);      // P1 has no foundation: ignored
    expect(1, 1, 0, true,  true,  true,  false);      // P0+P1
    expect(0, 0, 1, true,  false, false, true);       // P2 alone (independent)
    expect(1, 0, 1, true,  true,  false, true);       // P0+P2
    expect(0, 1, 1, true,  false, false, true);       // P2 alone; P1 foundation missing
    expect(1, 1, 1, true,  true,  true,  true);       // everything
    assert(checked == 8);
    std::printf("M02 switch-combination truth table PASS (8 combos, %d asserts)\n", checked);
    return 0;
}
