// Tests for the HOST-side blood decal basis: normal alignment, the parallel-normal
// fallback, and the world-space plane invariant that keeps both eyes on the same
// decal.
#include <cstdio>
#include <cstdlib>
#include <cmath>

#include "../../../spikes/view_matrix/vfx_blood.h"

using namespace singularity_vfx;

static int g_fail = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", msg); ++g_fail; }
}

static bool Perp(const VfxVec3& a, const VfxVec3& b, float eps = 1e-4f) {
    return std::fabs(VfxDot(a, b)) < eps;
}
static bool Unit(const VfxVec3& a, float eps = 1e-3f) {
    return std::fabs(VfxDot(a, a) - 1.0f) < eps;
}

int main() {
    // Basic orthonormal basis.
    VfxVec3 r{}, u{};
    Check(BuildSurfaceBasis({0, 0, 1}, {1, 0, 0}, r, u), "basis builds for a front-facing hit");
    Check(Unit(r) && Unit(u), "basis vectors are unit length");
    Check(Perp(r, {0, 0, 1}) && Perp(u, {0, 0, 1}), "both basis vectors lie in the surface plane");
    Check(Perp(r, u), "basis vectors are orthogonal");

    // Degenerate normal -> refuse (never draw a garbage decal).
    Check(!BuildSurfaceBasis({0, 0, 0}, {1, 0, 0}, r, u), "zero normal is rejected");
    Check(!BuildSurfaceBasis({NAN, 0, 0}, {1, 0, 0}, r, u), "NaN normal is rejected");

    // Parallel-normal fallback: hint parallel to the normal must still give a
    // stable, perpendicular basis (no NaN, no degenerate quad).
    Check(BuildSurfaceBasis({1, 0, 0}, {1, 0, 0}, r, u), "parallel hint uses the fallback");
    Check(Unit(r) && Unit(u), "fallback basis is unit length");
    Check(Perp(r, {1, 0, 0}) && Perp(u, {1, 0, 0}), "fallback basis is perpendicular to the normal");

    // World-space plane invariant: every corner lies in the surface plane through
    // the hit point, so the same world corners project for both eyes.
    const VfxVec3 hit{3.0f, 2.0f, -5.0f};
    const VfxVec3 normal = [&]{ VfxVec3 n; VfxNormalize({0.3f, -1.0f, 0.2f}, n, 1e-8f); return n; }();
    Check(BuildSurfaceBasis(normal, {1, 0, 0}, r, u), "basis builds for an angled hit");
    const float s = 0.05f;
    const VfxVec3 corners[4] = {
        {hit.x - r.x*s - u.x*s, hit.y - r.y*s - u.y*s, hit.z - r.z*s - u.z*s},
        {hit.x + r.x*s - u.x*s, hit.y + r.y*s - u.y*s, hit.z + r.z*s - u.z*s},
        {hit.x - r.x*s + u.x*s, hit.y - r.y*s + u.y*s, hit.z - r.z*s + u.z*s},
        {hit.x + r.x*s + u.x*s, hit.y + r.y*s + u.y*s, hit.z + r.z*s + u.z*s},
    };
    for (const VfxVec3& c : corners) {
        Check(std::fabs(VfxDot({c.x - hit.x, c.y - hit.y, c.z - hit.z}, normal)) < 1e-4f,
              "decal corner lies in the surface plane");
    }

    if (g_fail) { std::fprintf(stderr, "blood-basis tests: %d FAILED\n", g_fail); return 1; }
    std::puts("PASS: blood decal basis (normal alignment, fallback, world-plane invariant)");
    return 0;
}
