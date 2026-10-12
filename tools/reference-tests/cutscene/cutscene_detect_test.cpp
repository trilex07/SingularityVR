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
    moved.oldCameraLive = false;
    Check(migrated.Advance(Qualify(moved), moved), "dead-camera change disengages immediately");
    Check(!migrated.Engaged(), "disengaged after level transition");
    Check(migrated.LastClearReason() == cutscene::kCameraClear, "camera-clear reason recorded");

    // --- Matinee director cut: live old camera must NOT clear ---
    Latch directorCut;
    directorCut.Advance(Qualify(on), on);
    directorCut.Advance(Qualify(on), on);
    RawSignals cut{};
    cut.cinematicHook = true;
    cut.cameraChanged = true;
    cut.oldCameraLive = true;
    Check(!directorCut.Advance(Qualify(cut), cut), "director cut holds engagement");
    Check(directorCut.Engaged(), "still engaged through camera cut");
    Check(directorCut.Advance(Qualify(on), on) == false, "cinematic continues after cut");
    Check(directorCut.Engaged(), "engaged after cut with hook still asserted");

    // --- menu demotion holds the latch open (no menu-triggered flat) ---
    Latch menuLatch;
    menuLatch.Advance(Qualify(menuCine), menuCine);
    menuLatch.Advance(Qualify(menuCine), menuCine);
    Check(!menuLatch.Engaged(), "demoted menu signal never engages latch");

    // --- recorded-sequence replays (HISTORY/STATUS evidence, frame by frame) ---
    // Run-138 intro: CINE asserted through the intro, drops at gameplay start.
    {
        Latch intro;
        RawSignals f{};
        f.cinematicHook = true;
        bool changed = false;
        for (int i = 0; i < 10; ++i) changed = intro.Advance(Qualify(f), f);
        Check(intro.Engaged(), "run-138 replay: engaged during intro");
        RawSignals game{};
        for (int i = 0; i < 3; ++i) changed = intro.Advance(Qualify(game), game);
        Check(intro.Engaged(), "run-138 replay: hysteresis holds across 3 clear frames");
        changed = intro.Advance(Qualify(game), game);
        Check(changed && !intro.Engaged(), "run-138 replay: gameplay start restores stereo");
    }
    // Run-176 menu session: hook + menu focus for a long session, then a
    // level load (dead old camera). Never engages; transition cannot latch.
    {
        Latch menu;
        RawSignals f{};
        f.cinematicHook = true;
        f.menuFocus = true;
        for (int i = 0; i < 600; ++i) menu.Advance(Qualify(f), f);
        Check(!menu.Engaged(), "run-176 replay: 600 menu-CINE frames never engage");
        RawSignals load{};
        load.cameraChanged = true;
        load.oldCameraLive = false;
        menu.Advance(Qualify(load), load);
        Check(!menu.Engaged(), "run-176 replay: level load leaves latch disengaged");
    }
    // 92-ON / 88-OFF imbalance: session ends mid-cinematic (4 trailing ONs
    // with no OFF). Next launch changes camera with a dead old instance.
    {
        Latch trailing;
        RawSignals f{};
        f.cinematicHook = true;
        for (int i = 0; i < 4; ++i) trailing.Advance(Qualify(f), f);
        Check(trailing.Engaged(), "imbalance replay: trailing ONs engage");
        RawSignals relaunch{};
        relaunch.cameraChanged = true;
        relaunch.oldCameraLive = false;
        Check(trailing.Advance(Qualify(relaunch), relaunch), "relaunch transition clears");
        Check(!trailing.Engaged(), "no unmatched cinematic persists past relaunch");
    }
    // Bink: one confirmed quad engages immediately; four clear frames exit.
    {
        Latch bink;
        RawSignals f{};
        f.videoQuad = true;
        Check(bink.Advance(Qualify(f), f), "bink replay: single quad engages immediately");
        RawSignals g{};
        for (int i = 0; i < 3; ++i) bink.Advance(Qualify(g), g);
        Check(bink.Engaged(), "bink replay: holds across 3 clear frames");
        Check(bink.Advance(Qualify(g), g), "bink replay: fourth clear frame exits");
    }
    // Matinee with director cuts: hook holds across two live-old cuts, then
    // an explicit exit edge ends the cinematic immediately.
    {
        Latch matinee;
        RawSignals f{};
        f.cinematicHook = true;
        matinee.Advance(Qualify(f), f);
        matinee.Advance(Qualify(f), f);
        for (int cut = 0; cut < 2; ++cut) {
            RawSignals c{};
            c.cinematicHook = true;
            c.cameraChanged = true;
            c.oldCameraLive = true;
            matinee.Advance(Qualify(c), c);
            Check(matinee.Engaged(), "matinee replay: survives director cut");
            matinee.Advance(Qualify(f), f);
        }
        RawSignals exit{};
        exit.cinematicExit = true;
        Check(matinee.Advance(Qualify(exit), exit), "matinee replay: exit edge ends at once");
        Check(!matinee.Engaged(), "matinee replay: disengaged after exit");
    }
    // Pause during an engaged cinematic: pause is diagnostic, latch follows
    // the hook, not the pause flag.
    {
        Latch paused;
        RawSignals f{};
        f.cinematicHook = true;
        f.paused = true;
        paused.Advance(Qualify(f), f);
        paused.Advance(Qualify(f), f);
        Check(paused.Engaged(), "pause replay: hook still engages while paused");
    }

    if (g_fail) { std::fprintf(stderr, "cutscene detect: %d FAILED\n", g_fail); return 1; }
    std::puts("PASS: cutscene detect (qualify + latch + recorded replays)");
    return 0;
}
