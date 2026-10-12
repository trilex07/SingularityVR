// Offline tests for the cutscene detection voter (no D3D, no game, no Steam).
// Research branch: production d3d9.cpp and vr_scene_mode.h are untouched.
//
// Build (x86 MSVC): cl /nologo /EHsc /W3 /std:c++17
//   tools\reference-tests\cutscene\cutscene_detect_test.cpp
#include <cstdio>

#include "../../../spikes/view_matrix/cutscene_detect.h"

static int g_fail = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", msg); ++g_fail; }
}

using cutscene::Latch;
using cutscene::Qualify;
using cutscene::RawSignals;
using cutscene::Vote;

int main() {
    // --- one-frame qualification ---
    RawSignals play{};
    Vote v0 = Qualify(play);
    Check(!v0.cinematic, "gameplay votes non-cinematic");

    RawSignals cine{};
    cine.cinematicHook = true;
    Vote v1 = Qualify(cine);
    Check(v1.cinematic && (v1.reasons & cutscene::kHook), "hook alone votes cinematic");

    RawSignals menuCine{};
    menuCine.cinematicHook = true;
    menuCine.menuFocus = true;
    Vote v2 = Qualify(menuCine);
    Check(!v2.cinematic && (v2.reasons & cutscene::kMenuDemote),
          "menu-context hook demoted (run-176 false positive)");

    RawSignals menuVideo{};
    menuVideo.cinematicHook = true;
    menuVideo.menuFocus = true;
    menuVideo.videoQuad = true;
    Vote v3 = Qualify(menuVideo);
    Check(v3.cinematic && (v3.reasons & cutscene::kVideo),
          "video corroboration overrides menu demotion");

    RawSignals videoOnly{};
    videoOnly.videoQuad = true;
    Vote v4 = Qualify(videoOnly);
    Check(v4.cinematic, "confirmed video quad votes cinematic");

    RawSignals pausedOnly{};
    pausedOnly.paused = true;
    Check(!Qualify(pausedOnly).cinematic, "pause alone never cinematic");

    // --- latch entry/exit hysteresis ---
    Latch latch;
    RawSignals on{};
    on.cinematicHook = true;
    Check(!latch.Advance(Qualify(on), on), "single noisy sample cannot engage");
    Check(!latch.Engaged(), "latch still disengaged after one sample");
    Check(latch.Advance(Qualify(on), on), "second consecutive sample engages");
    Check(latch.Engaged(), "latch engaged");

    RawSignals off{};
    Check(!latch.Advance(Qualify(off), off), "one clear frame holds engagement");
    Check(!latch.Advance(Qualify(off), off), "two clear frames hold engagement");
    Check(!latch.Advance(Qualify(off), off), "three clear frames hold engagement");
    Check(latch.Engaged(), "still engaged before fourth clear frame");
    Check(latch.Advance(Qualify(off), off), "fourth clear frame disengages");
    Check(!latch.Engaged(), "latch disengaged");
    Check(latch.LastClearReason() == cutscene::kHysteresis, "hysteresis clear reason recorded");

    // --- explicit clear paths (run-211 lesson: never latch without one) ---
    Latch latched;
    latched.Advance(Qualify(on), on);
    latched.Advance(Qualify(on), on);
    RawSignals exitEdge{};
    exitEdge.cinematicHook = false;
    exitEdge.cinematicExit = true;
    Check(latched.Advance(Qualify(exitEdge), exitEdge), "exit edge disengages immediately");
    Check(!latched.Engaged(), "disengaged after exit edge");
    Check(latched.LastClearReason() == cutscene::kExitEdge, "exit-edge clear reason recorded");

    Latch migrated;
    migrated.Advance(Qualify(on), on);
    migrated.Advance(Qualify(on), on);
    RawSignals moved{};
    moved.cameraChanged = true;
    Check(migrated.Advance(Qualify(moved), moved), "camera change disengages immediately");
    Check(!migrated.Engaged(), "disengaged after camera change");
    Check(migrated.LastClearReason() == cutscene::kCameraClear, "camera-clear reason recorded");

    // --- menu demotion holds the latch open (no menu-triggered flat) ---
    Latch menuLatch;
    menuLatch.Advance(Qualify(menuCine), menuCine);
    menuLatch.Advance(Qualify(menuCine), menuCine);
    Check(!menuLatch.Engaged(), "demoted menu signal never engages latch");

    if (g_fail) { std::fprintf(stderr, "cutscene detect: %d FAILED\n", g_fail); return 1; }
    std::puts("PASS: cutscene detect (qualify + hysteresis + clear paths + menu demotion)");
    return 0;
}
