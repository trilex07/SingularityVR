// Policy helper for the proposed menu-classification early-out.
//
// The production draw hooks call MenuDrawTraceRecord and SuppressCapturedVrMenuDraw on every
// draw even when no menu is focused.  The existing suppression logic already returns false in
// that state, so moving the guard one level higher changes no behavior but removes per-draw
// work from the dominant gameplay path.
//
// This file implements the guard in isolation and tests it against the state combinations
// described in native_ui_input.h / vr_menu_overlay.h.
//
// Build:
//   cl /EHsc /W3 /O2 /std:c++17 test-menu-classify-policy.cpp
//   g++ -std=c++17 -O2 -o test-menu-classify-policy test-menu-classify-policy.cpp

#include <cstdio>
#include <cstdlib>

// Matches the semantics of g_vrUiMenu (0/1) and g_vrMenuClass (0 = none, 1-4 = focused).
// captureCanProvide mirrors g_vrCaptureOwner.CanProvide(VrMenuStateToken()).
inline bool ShouldSkipMenuClassification(bool vrUiMenu, int menuClass,
                                          bool captureCanProvide) {
    return !vrUiMenu && menuClass == 0 && !captureCanProvide;
}

static int g_failures = 0;

#define CHECK(cond, msg) do { \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL: %s\n", msg); \
        ++g_failures; \
    } \
} while (0)

static void test_gameplay_default_skips() {
    // The vast majority of gameplay draws: no menu, no class, no provider capture.
    CHECK(ShouldSkipMenuClassification(false, 0, false),
          "gameplay default should skip menu classification");
}

static void test_focused_menu_never_skips() {
    for (int cls = 1; cls <= 4; ++cls) {
        CHECK(!ShouldSkipMenuClassification(true, cls, false),
              "focused menu must not skip classification");
        CHECK(!ShouldSkipMenuClassification(false, cls, false),
              "non-zero menu class must not skip classification even if vrUiMenu is false");
    }
}

static void test_provider_active_never_skips() {
    // Capture owner may still be drawing the head-relative overlay after the menu movie
    // reports it is no longer focused, until the next frame invalidates it.
    CHECK(!ShouldSkipMenuClassification(false, 0, true),
          "active provider capture must not skip classification");
}

static void test_all_combinations_exhaustively() {
    // truth table: vrUiMenu ∈ {false,true}, menuClass ∈ {0,1,2,3,4}, capture ∈ {false,true}
    int skipCount = 0;
    for (int ui = 0; ui <= 1; ++ui) {
        for (int cls = 0; cls <= 4; ++cls) {
            for (int cap = 0; cap <= 1; ++cap) {
                bool skip = ShouldSkipMenuClassification(ui != 0, cls, cap != 0);
                if (skip) ++skipCount;
                // Only the single gameplay-default triple may skip.
                bool expectedSkip = (ui == 0 && cls == 0 && cap == 0);
                if (skip != expectedSkip) {
                    std::fprintf(stderr,
                        "FAIL: state (ui=%d class=%d cap=%d) skip=%d expected=%d\n",
                        ui, cls, cap, skip, expectedSkip);
                    ++g_failures;
                }
            }
        }
    }
    CHECK(skipCount == 1, "exactly one state combination should skip");
}

int main() {
    test_gameplay_default_skips();
    test_focused_menu_never_skips();
    test_provider_active_never_skips();
    test_all_combinations_exhaustively();

    if (g_failures == 0) {
        std::printf("OK: all menu-classify policy tests passed\n");
        return 0;
    }
    std::fprintf(stderr, "FAILED: %d test(s)\n", g_failures);
    return 1;
}
