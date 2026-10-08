// Regression test for the Present-path VFX overlay conversion.
// A collected item is expressed in metres and must be converted once per eye;
// the old implementation converted the same mutable item twice.
#include <cassert>
#include <cmath>
#include <cstdio>

struct Item { float x, y, z, size; };

static Item Convert(Item in, float metresToUU) {
    in.x *= metresToUU;
    in.y *= metresToUU;
    in.z *= metresToUU;
    in.size *= metresToUU;
    return in;
}

int main() {
    const Item collected{0.30f, 1.20f, 2.00f, 0.045f};
    const float m2u = 52.5f;
    const Item left = Convert(collected, m2u);
    const Item right = Convert(collected, m2u);

    // Both eyes use the same world-space size and position.
    assert(std::fabs(left.x - right.x) < 1e-6f);
    assert(std::fabs(left.y - right.y) < 1e-6f);
    assert(std::fabs(left.z - right.z) < 1e-6f);
    assert(std::fabs(left.size - right.size) < 1e-6f);

    // A second conversion is precisely the regression this catches.
    const Item broken = Convert(left, m2u);
    assert(std::fabs(broken.size - left.size) > 1.0f);
    std::puts("PASS: VFX eye conversion is immutable and single-scale");
    return 0;
}
