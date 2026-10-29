// Policy regression tests for the proposed opt-in/default-off MenuDrawTrace behavior.
//
// MenuDrawTraceRecord is a pure diagnostic: it records draw state into fixed buffers
// and flushes them on menu state transitions.  Disabling it (g_menuDrawTrace=0) must
// not affect capture cleanup, menu suppression, or any native UI behavior.
//
// This file models the gate in menu_draw_trace.h:55 in isolation and verifies that:
//   - enabled traces record groups during the sampling window;
//   - disabled traces record nothing;
//   - the disabled path is one early return, not a partial record.
//
// Build:
//   cl /EHsc /W3 /O2 /std:c++17 test-menu-trace-policy.cpp
//   g++ -std=c++17 -O2 -o test-menu-trace-policy test-menu-trace-policy.cpp

#include <cstdio>
#include <cstdlib>
#include <cstring>

static int g_failures = 0;

#define CHECK(cond, msg) do { \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL: %s\n", msg); \
        ++g_failures; \
    } \
} while (0)

// Minimal model of MenuDrawTraceRecord's gate and a single fixed-size group counter.
// In production the real function returns at line 55 when g_menuDrawTrace == 0.
struct TraceModel {
    bool enabled = false;
    int frame = 0;
    int startFrame = 0;
    unsigned long long state = 0;
    unsigned long long lastState = ~0ull;
    int groupsRecorded = 0;
    int queriesSimulated = 0;
    int stateReads = 0;

    void setEnabled(bool v) { enabled = v; }

    void onStateChange(unsigned long long newState) {
        ++stateReads;
        if (newState != lastState) {
            // Flush implied; reset for next sampling window.
            groupsRecorded = 0;
            queriesSimulated = 0;
            lastState = newState;
            startFrame = frame;
        }
    }

    void recordDraw() {
        if (!enabled) {
            // Early return path: zero diagnostic work.
            return;
        }
        const int age = frame - startFrame;
        if (age < 20 || age >= 28) {
            return;
        }
        // Simulate the ~20 D3D9 queries the real function issues per sampled draw.
        queriesSimulated += 20;
        if (groupsRecorded < 2048) {
            ++groupsRecorded;
        }
    }

    void advanceFrame() { ++frame; }
    void onFrame(unsigned long long newState) {
        if (!enabled) return; // Same early gate as production MenuDrawTraceFrame.
        advanceFrame();
        onStateChange(newState);
    }
};

static void test_disabled_trace_records_nothing() {
    TraceModel t;
    CHECK(!t.enabled, "diagnostic census must default off");
    for (int i = 0; i < 40; ++i) {
        t.onFrame(12345);
        t.recordDraw();
    }
    CHECK(t.groupsRecorded == 0, "disabled trace must record zero groups");
    CHECK(t.queriesSimulated == 0, "disabled trace must issue zero D3D queries");
    CHECK(t.stateReads == 0, "disabled trace must avoid menu token/state work");
}

static void test_enabled_trace_records_in_window_only() {
    TraceModel t;
    t.setEnabled(true);
    t.onStateChange(12345);
    for (int i = 0; i < 40; ++i) {
        t.advanceFrame();
        t.recordDraw();
    }
    // Window is frames 20-27 inclusive = 8 frames, one group per draw in this model.
    CHECK(t.groupsRecorded == 8, "enabled trace must record exactly the 8-frame window");
    CHECK(t.queriesSimulated == 8 * 20, "enabled trace must issue queries only in window");
}

static void test_state_change_resets_window() {
    TraceModel t;
    t.setEnabled(true);
    t.onStateChange(1);
    for (int i = 0; i < 27; ++i) {
        t.advanceFrame();
        t.recordDraw();
    }
    // Frames 20-27 inclusive fall inside the 8-frame window.
    CHECK(t.groupsRecorded == 8, "first window should record frames 20-27");
    t.onStateChange(2);
    // State change flushes and resets; new window starts at current frame.
    CHECK(t.groupsRecorded == 0, "state change must reset recorded groups");
}

static void test_disabled_preserves_state_machine() {
    // Disabled diagnostic must not process unrelated native menu transitions.
    TraceModel t;
    t.onFrame(1);
    t.onFrame(2);
    t.onFrame(3);
    CHECK(t.stateReads == 0, "disabled trace must not read menu state");
    CHECK(t.groupsRecorded == 0, "no groups recorded while disabled");
}

int main() {
    test_disabled_trace_records_nothing();
    test_enabled_trace_records_in_window_only();
    test_state_change_resets_window();
    test_disabled_preserves_state_machine();

    if (g_failures == 0) {
        std::printf("OK: all menu-trace policy tests passed\n");
        return 0;
    }
    std::fprintf(stderr, "FAILED: %d test(s)\n", g_failures);
    return 1;
}
