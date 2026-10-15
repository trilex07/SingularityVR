#include "../pose_conversion.h"
#include <cmath>
#include <cstdio>

static int g_fails = 0;
static void Expect(bool cond, const char* name) {
    if (cond) { std::printf("PASS %s\n", name); return; }
    std::printf("FAIL %s\n", name);
    ++g_fails;
}
static bool Close(float a, float b) { return std::fabs(a - b) < 1e-4f; }

// Inverse of TrackingDeltaToWorldUU, as used by the holster marker to turn
// head-relative hip UU into absolute XR metres. Must roundtrip exactly.
static vrpose::V3 Invert(vrpose::V3 w, float cy, float by, float u) {
    const float bc = std::cos(by), bs = std::sin(by);
    const float cc = std::cos(cy), sc = std::sin(cy);
    const float XN = w.x / u, YN = w.y / u;
    const float fwd = XN * bc + YN * bs;
    const float rgt = -XN * bs + YN * bc;
    return {-sc * fwd + cc * rgt, w.z / u, -cc * fwd - sc * rgt};
}

int main() {
    using namespace vrpose;
    const float units[] = {50.0f, 62.5f, 40.0f};
    const float yaws[] = {0.0f, 0.7f, -2.4f, 3.14159265f, 1.2f};
    const V3 vecs[] = {{0.3f, -0.5f, 0.8f}, {-0.2f, 0.1f, -0.4f},
                       {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}};
    bool ok = true;
    for (float u : units)
        for (float cy : yaws)
            for (float by : yaws)
                for (V3 m : vecs) {
                    const V3 w = TrackingDeltaToWorldUU(m, cy, by, u);
                    const V3 b = Invert(w, cy, by, u);
                    if (!(Close(b.x, m.x) && Close(b.y, m.y) && Close(b.z, m.z)))
                        ok = false;
                }
    Expect(ok, "uu<->xr roundtrip");
    // Documented axis convention at zero yaw: fwd=-mz, right=+mx, up=+my.
    {
        const V3 w = TrackingDeltaToWorldUU({0.5f, 0.25f, -0.75f}, 0.0f, 0.0f, 50.0f);
        Expect(Close(w.x, 0.75f * 50.0f) && Close(w.y, 0.5f * 50.0f) &&
               Close(w.z, 0.25f * 50.0f), "zero-yaw axes");
    }
    if (g_fails == 0) std::puts("vrpose: invert L1");
    return g_fails == 0 ? 0 : 1;
}
