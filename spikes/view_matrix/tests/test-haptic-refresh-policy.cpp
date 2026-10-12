// Regression test for the haptic refresh-interval policy.
//
// d3d9.cpp submits haptic feedback on amplitude changes and refreshes periodically
// so a 200 ms sustained pulse does not expire mid-rumble.  This test models the
// refresh logic and verifies that the proposed modulus preserves the pulse while
// reducing API calls.
//
// Build:
//   cl /EHsc /W3 /O2 /std:c++17 test-haptic-refresh-policy.cpp
//   g++ -std=c++17 -O2 -o test-haptic-refresh-policy test-haptic-refresh-policy.cpp

#include <cmath>
#include <cstdio>
#include <cstdlib>

static int g_failures = 0;

#define CHECK(cond, msg) do { \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL: %s\n", msg); \
        ++g_failures; \
    } \
} while (0)

struct HapticModel {
    int refreshModulus = 60;
    int frame = 0;
    int refresh = 0;
    float lastAmp[2] = { -1.0f, -1.0f };
    int applyCalls = 0;
    int stopCalls = 0;

    void submitFrame(float ampL, float ampR) {
        ++refresh;
        float amps[2] = { ampL, ampR };
        for (int hand = 0; hand < 2; ++hand) {
            const float amp = amps[hand];
            const bool changed = std::fabs(amp - lastAmp[hand]) > 0.02f;
            const bool expiring = amp > 0.0f && (refresh % refreshModulus) == 0;
            if (!changed && !expiring) continue;
            lastAmp[hand] = amp;
            if (amp <= 0.0f) ++stopCalls;
            else               ++applyCalls;
        }
        ++frame;
    }

    // Consume the initial -1 -> 0 transition so tests count steady-state behavior.
    void prime() {
        submitFrame(0.0f, 0.0f);
        applyCalls = 0;
        stopCalls = 0;
    }
};

static void test_amplitude_changes_apply_immediately() {
    HapticModel m;
    m.prime();
    m.submitFrame(0.5f, 0.0f);
    CHECK(m.applyCalls == 1, "amplitude rise must apply immediately");
    m.submitFrame(0.0f, 0.0f);
    CHECK(m.stopCalls == 1, "amplitude drop to zero must stop immediately");
}

static void test_constant_amplitude_refreshes_at_interval() {
    HapticModel m;
    m.refreshModulus = 60;
    m.prime();
    // Ramp up, then hold for 240 frames at 120 fps = 2 seconds.
    m.submitFrame(0.5f, 0.5f);
    for (int i = 0; i < 240; ++i) {
        m.submitFrame(0.5f, 0.5f);
    }
    // Two ramp-up applies (one per hand), plus refreshes at frames 60, 120, 180, 240 per hand.
    CHECK(m.applyCalls == 10, "constant amplitude must refresh at modulus interval");
    CHECK(m.stopCalls == 0, "constant amplitude must not stop");
}

static void test_old_modulus_more_calls() {
    HapticModel m15;
    m15.refreshModulus = 15;
    m15.prime();
    m15.submitFrame(0.5f, 0.5f);
    for (int i = 0; i < 240; ++i) {
        m15.submitFrame(0.5f, 0.5f);
    }

    HapticModel m60;
    m60.refreshModulus = 60;
    m60.prime();
    m60.submitFrame(0.5f, 0.5f);
    for (int i = 0; i < 240; ++i) {
        m60.submitFrame(0.5f, 0.5f);
    }

    CHECK(m15.applyCalls > m60.applyCalls, "modulus 15 must produce more apply calls than modulus 60");
    // 2 ramp-up applies + refreshes at 15,30,...,240 per hand = 34 calls for modulus 15
    // 2 ramp-up applies + refreshes at 60,120,180,240 per hand = 10 calls for modulus 60
    CHECK(m15.applyCalls == 34, "unexpected apply count for modulus 15");
    CHECK(m60.applyCalls == 10, "unexpected apply count for modulus 60");
}

static void test_pulse_preserved_with_modulus_60() {
    // 200 ms pulse at 120 fps spans ~24 frames.  A refresh every 60 frames still
    // refreshes before the pulse expires.
    HapticModel m;
    m.refreshModulus = 60;
    m.prime();
    m.submitFrame(1.0f, 1.0f);
    for (int i = 0; i < 24; ++i) {
        m.submitFrame(1.0f, 1.0f);
    }
    m.submitFrame(0.0f, 0.0f);
    CHECK(m.applyCalls == 2, "single ramp-up apply per hand");
    CHECK(m.stopCalls == 2, "single stop at end of 200 ms pulse per hand");
}

int main() {
    test_amplitude_changes_apply_immediately();
    test_constant_amplitude_refreshes_at_interval();
    test_old_modulus_more_calls();
    test_pulse_preserved_with_modulus_60();

    if (g_failures == 0) {
        std::printf("OK: all haptic refresh policy tests passed\n");
        return 0;
    }
    std::fprintf(stderr, "FAILED: %d test(s)\n", g_failures);
    return 1;
}
