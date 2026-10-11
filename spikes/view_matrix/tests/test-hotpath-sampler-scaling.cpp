// Extended scaling tests for spikes/view_matrix/hotpath_sampler.h.
//
// The existing test-hotpath-sampler.cpp covers windowing, delay activation and the
// 1-in-32 sampling stride.  This file adds deterministic verification of the
// milliseconds_per_frame / microseconds_per_call scaling formula.
//
// Build:
//   cl /EHsc /W3 /O2 /std:c++17 test-hotpath-sampler-scaling.cpp
//   g++ -std=c++17 -O2 -o test-hotpath-sampler-scaling test-hotpath-sampler-scaling.cpp

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../hotpath_sampler.h"
#include <chrono>
#include <cmath>
#include <cstdio>

static int g_failures = 0;

#define CHECK(cond, msg) do { \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL: %s\n", msg); \
        ++g_failures; \
    } \
} while (0)

static void busy_spin_us(int us) {
    const auto t0 = std::chrono::high_resolution_clock::now();
    while (true) {
        auto dt = std::chrono::high_resolution_clock::now() - t0;
        if (std::chrono::duration_cast<std::chrono::microseconds>(dt).count() >= us)
            break;
    }
}

static void test_scaling_formula() {
    // Use a window of exactly 1 frame and a call count that is a multiple of the
    // 32-call sampling stride so the sample count is exact.
    hotpath::sampler s;
    s.configure(true, 1, 0);

    const int calls = 320;  // 10 samples exactly
    for (int i = 0; i < calls; ++i) {
        hotpath::scope sc(s, hotpath::Family::draw_hook);
        busy_spin_us(10);
    }

    CHECK(s.advance_frame(), "single-frame window should end");

    const auto r = s.read(hotpath::Family::draw_hook);
    CHECK(r.calls == static_cast<std::uint64_t>(calls), "call count mismatch");
    CHECK(r.samples == 10, "sample count should be calls/32");

    const double us = s.microseconds_per_call(hotpath::Family::draw_hook);
    const double msf = s.milliseconds_per_frame(hotpath::Family::draw_hook, 1);

    CHECK(us > 0.0, "microseconds_per_call must be positive");
    CHECK(msf > 0.0, "milliseconds_per_frame must be positive");

    // ms/frame for a single-frame window == us/call * calls / 1000
    const double expected_msf = us * static_cast<double>(calls) / 1000.0;
    const double ratio = (expected_msf > 0.0) ? (msf / expected_msf) : 0.0;
    CHECK(std::fabs(ratio - 1.0) < 0.05, "ms/frame scaling mismatch");
}

static void test_zero_samples_return_zero() {
    hotpath::sampler s;
    s.configure(true, 1, 0);
    CHECK(s.advance_frame(), "window should end");
    CHECK(s.microseconds_per_call(hotpath::Family::draw_hook) == 0.0,
          "empty sampler should return 0 us/call");
    CHECK(s.milliseconds_per_frame(hotpath::Family::draw_hook, 1) == 0.0,
          "empty sampler should return 0 ms/frame");
}

int main() {
    test_scaling_formula();
    test_zero_samples_return_zero();

    if (g_failures == 0) {
        std::printf("OK: all hotpath sampler scaling tests passed\n");
        return 0;
    }
    std::fprintf(stderr, "FAILED: %d test(s)\n", g_failures);
    return 1;
}
