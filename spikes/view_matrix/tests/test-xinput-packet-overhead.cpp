// Microbenchmark / regression test for the XInput packet-number logic.
//
// Hook_XInputGetState uses memcmp + a packet-number deadzone to let UE3 skip its
// full input path when the pad state has not meaningfully changed.  This file
// measures that logic in isolation and verifies that small stick noise does not
// advance the packet.
//
// Build:
//   cl /EHsc /W3 /O2 /std:c++17 test-xinput-packet-overhead.cpp
//   g++ -std=c++17 -O2 -o test-xinput-packet-overhead test-xinput-packet-overhead.cpp

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <chrono>

// Minimal XINPUT_GAMEPAD layout matching d3d9.cpp's XiGamepad.
struct Gamepad {
    uint16_t wButtons;
    uint8_t  bLeftTrigger;
    uint8_t  bRightTrigger;
    int16_t  sThumbLX;
    int16_t  sThumbLY;
    int16_t  sThumbRX;
    int16_t  sThumbRY;
};

static int g_failures = 0;

#define CHECK(cond, msg) do { \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL: %s\n", msg); \
        ++g_failures; \
    } \
} while (0)

static bool ApplyDeadzone(int16_t& x, int16_t& y) {
    // Matches the run-70 radial deadzone described in HISTORY.md: small stick
    // noise is silenced so the packet stops advancing at rest.
    const long dx = x, dy = y;
    const long r2 = dx * dx + dy * dy;
    const long dead2 = 8000L * 8000L;
    if (r2 < dead2) { x = 0; y = 0; return true; }
    return false;
}

static bool StateChanged(const Gamepad& cur, const Gamepad& last) {
    return std::memcmp(&cur, &last, sizeof(Gamepad)) != 0;
}

static void test_deadzone_silences_noise() {
    Gamepad last{};
    Gamepad cur{};
    int advances = 0;
    for (int i = 0; i < 1000; ++i) {
        // Simulate +/- 1000 units of stick noise.
        cur.sThumbLX = static_cast<int16_t>((i % 2001) - 1000);
        cur.sThumbLY = static_cast<int16_t>((i % 1999) - 999);
        ApplyDeadzone(cur.sThumbLX, cur.sThumbLY);
        if (StateChanged(cur, last)) {
            last = cur;
            ++advances;
        }
    }
    CHECK(advances == 0, "small stick noise must not advance packet");
}

static void test_large_stick_change_advances() {
    Gamepad last{};
    Gamepad cur{};
    cur.sThumbLX = 30000;
    ApplyDeadzone(cur.sThumbLX, cur.sThumbLY);
    CHECK(StateChanged(cur, last), "large stick deflection must advance packet");
}

static void test_button_press_advances() {
    Gamepad last{};
    Gamepad cur{};
    cur.wButtons = 0x1000;  // A button
    CHECK(StateChanged(cur, last), "button press must advance packet");
}

static void benchmark_packet_compare() {
    Gamepad a{}, b{};
    b.sThumbLX = 1;
    const auto t0 = std::chrono::high_resolution_clock::now();
    volatile bool diff = false;
    for (int i = 0; i < 1000000; ++i) {
        diff = std::memcmp(&a, &b, sizeof(Gamepad)) != 0;
    }
    const auto dt = std::chrono::high_resolution_clock::now() - t0;
    const double us = std::chrono::duration_cast<std::chrono::nanoseconds>(dt).count() / 1000.0;
    std::printf("packet compare benchmark: %.3f us per 1M compares (diff=%d)\n", us, diff ? 1 : 0);
}

int main() {
    test_deadzone_silences_noise();
    test_large_stick_change_advances();
    test_button_press_advances();
    benchmark_packet_compare();

    if (g_failures == 0) {
        std::printf("OK: all XInput packet-overhead tests passed\n");
        return 0;
    }
    std::fprintf(stderr, "FAILED: %d test(s)\n", g_failures);
    return 1;
}
