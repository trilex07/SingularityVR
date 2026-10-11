#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include "../pause_gfx_signature.h"
#include "../pause_gfx_transform.h"

int main() {
    using namespace pausegfx;
    assert(FlashStrideSlot(12) == 0);
    assert(FlashStrideSlot(8) == 1);
    assert(FlashStrideSlot(32) < 0);

    float mainC[16];
    float pauseC[16];
    MdgMainMenuFlashC5(mainC);
    MdgPauseFlashC5(pauseC);
    assert(FlashMatrixShape(mainC));
    assert(FlashMatrixShape(pauseC));

    constexpr float sceneW = 5264.f;
    constexpr float sceneH = 2684.f;
    constexpr float poolW = 1280.f;
    constexpr float poolH = 720.f;
    const float sx = poolW / sceneW;
    const float sy = poolH / sceneH;

    float mapped[16];
    std::memcpy(mapped, pauseC, sizeof(mapped));
    RemapFlashC5SceneSbsToPool(mapped, sceneW, sceneH, poolW, poolH);
    assert(std::fabs(mapped[12] - pauseC[12] * sx) < 1e-6f);
    assert(std::fabs(mapped[13] - pauseC[13] * sy) < 1e-6f);
    assert(std::fabs(mapped[0] - pauseC[0] * sx) < 1e-9f);
    assert(std::fabs(mapped[5] - pauseC[5] * sy) < 1e-9f);

    float cx = 0.f, cy = 0.f;
    FlashRowClipXY(pauseC, 0.f, 0.f, cx, cy);
    assert(std::fabs(cx - pauseC[12]) < 1e-6f);
    assert(std::fabs(cy - pauseC[13]) < 1e-6f);
    float mx = 0.f, my = 0.f;
    FlashRowClipXY(mapped, 0.f, 0.f, mx, my);
    assert(std::fabs(mx - mapped[12]) < 1e-6f);
    assert(std::fabs(my - mapped[13]) < 1e-6f);

    const float corners[4][2] = {{-1.f, -1.f}, {1.f, -1.f}, {-1.f, 1.f}, {1.f, 1.f}};
    for (int i = 0; i < 4; ++i) {
        float px = 0.f, py = 0.f, qx = 0.f, qy = 0.f;
        FlashRowClipXY(pauseC, corners[i][0], corners[i][1], px, py);
        FlashRowClipXY(mapped, corners[i][0], corners[i][1], qx, qy);
        assert(std::fabs(qx - px * sx) < 2e-4f);
        assert(std::fabs(qy - py * sy) < 2e-4f);
    }

    RECT r{100, 200, 900, 600};
    RemapScissorSceneSbsToPool(r, sceneW, sceneH, poolW, poolH);
    assert(r.left == (LONG)lroundf(100.f * sx));
    assert(r.right == (LONG)lroundf(900.f * sx));
    assert(r.top == (LONG)lroundf(200.f * sy));
    assert(r.bottom == (LONG)lroundf(600.f * sy));

    std::puts("pause gfx: structural slots, MDG matrix shape, SBS->pool remap, scissor");
    return 0;
}
