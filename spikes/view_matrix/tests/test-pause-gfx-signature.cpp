#include <cassert>
#include <cmath>
#include <cstdio>
#include "../pause_gfx_signature.h"
#include "../pause_gfx_transform.h"

static void ExpectNear(float got, float want, float tol, const char* label) {
    if (std::fabs(got - want) > tol) {
        std::printf("FAIL %s got=%.4f want=%.4f\n", label, got, want);
        assert(false);
    }
}

int main() {
    using namespace pausegfx;
    assert(FlashStrideSlot(12) == 0);
    assert(FlashStrideSlot(8) == 1);
    assert(FlashStrideSlot(32) < 0);

    // MDG 137 (pause retest): kind=3 stride 12, vp 0,0,5264,2684.
    // xy floats are int16 pairs. y=-1755 is unique; x is the midpoint of the
    // int16 range whose float32 prints as the logged %.4g value.
    float pauseC[16];
    MdgPause137FlashC5(pauseC);
    assert(FlashMatrixShape(pauseC));
    constexpr int kY = -1755;
    constexpr int kXFirst = 19071;  // midpoint of [18062, 20081]
    constexpr int kXLast = -18191;  // midpoint of [-19201, -17182]
    constexpr float kSceneW = 5264.f;
    constexpr float kSceneH = 2684.f;
    constexpr float kPoolW = 1280.f;
    constexpr float kPoolH = 720.f;

    float srcX = 0.f, srcY = 0.f;
    ProjectFlashInt16ToPixel(pauseC, kXFirst, kY, 0.f, 0.f, kSceneW, kSceneH, srcX, srcY);
    ExpectNear(srcX, 1616.57f, 0.05f, "mdg137 source x");
    ExpectNear(srcY, 541.28f, 0.05f, "mdg137 source y");

    float srcLastX = 0.f, srcLastY = 0.f;
    ProjectFlashInt16ToPixel(pauseC, kXLast, kY, 0.f, 0.f, kSceneW, kSceneH, srcLastX, srcLastY);
    const float srcSpan = std::fabs(srcX - srcLastX);
    ExpectNear(srcSpan, 1309.28f, 0.1f, "mdg137 source span");

    float mapped[16];
    std::memcpy(mapped, pauseC, sizeof(mapped));
    RemapFlashC5Contain(mapped, kSceneW, kSceneH, kPoolW, kPoolH);
    float dstX = 0.f, dstY = 0.f, dstLastX = 0.f, dstLastY = 0.f;
    ProjectFlashInt16ToPixel(mapped, kXFirst, kY, 0.f, 0.f, kPoolW, kPoolH, dstX, dstY);
    ProjectFlashInt16ToPixel(mapped, kXLast, kY, 0.f, 0.f, kPoolW, kPoolH, dstLastX, dstLastY);
    ExpectNear(dstX, 393.09f, 0.05f, "mdg137 pool x");
    ExpectNear(dstY, 165.30f, 0.05f, "mdg137 pool y");
    ExpectNear(dstLastX, 74.72f, 0.05f, "mdg137 pool last x");
    ExpectNear(dstLastY, 165.30f, 0.05f, "mdg137 pool last y");
    const float dstSpan = std::fabs(dstX - dstLastX);
    ExpectNear(dstSpan, 318.37f, 0.08f, "mdg137 pool span");
    ExpectNear(dstSpan / srcSpan, 1280.f / 5264.f, 0.0001f, "span scale");

    // Viewport center stays the pool center. Scene corners letterbox, they do not stretch to 720.
    float cx = 0.f, cy = 0.f;
    ClipToViewportPixel(0.f, 0.f, 0.f, 0.f, kPoolW, kPoolH, cx, cy);
    ExpectNear(cx, 640.f, 0.01f, "pool center x");
    ExpectNear(cy, 360.f, 0.01f, "pool center y");
    const FlashContainMap contain = MakeFlashContainMap(kSceneW, kSceneH, kPoolW, kPoolH);
    float topX = 0.f, topY = 0.f, botX = 0.f, botY = 0.f;
    MapSourcePixelContain(contain, 0.f, 0.f, topX, topY);
    MapSourcePixelContain(contain, kSceneW, kSceneH, botX, botY);
    ExpectNear(topX, 0.f, 0.01f, "letterbox left");
    ExpectNear(botX, 1280.f, 0.01f, "letterbox right");
    ExpectNear(topY, 33.68f, 0.02f, "letterbox top");
    ExpectNear(botY, 686.32f, 0.02f, "letterbox bottom");
    assert(botY < kPoolH - 1.f);

    RECT full{0, 0, 5264, 2684};
    RemapScissorContain(full, kSceneW, kSceneH, kPoolW, kPoolH);
    assert(full.left == 0 && full.right == 1280);
    assert(full.top == 34 && full.bottom == 686);
    RECT partial{100, 200, 900, 600};
    RemapScissorContain(partial, kSceneW, kSceneH, kPoolW, kPoolH);
    assert(partial.left == 24 && partial.top == 82);
    assert(partial.right == 219 && partial.bottom == 180);

    // Same vertices, later MDG 137 sample (c13=0.3083): the row moves on the source and on the pool.
    float moved[16];
    std::memcpy(moved, pauseC, sizeof(moved));
    moved[13] = 0.3083f;
    float movedSrcX = 0.f, movedSrcY = 0.f;
    ProjectFlashInt16ToPixel(moved, kXFirst, kY, 0.f, 0.f, kSceneW, kSceneH, movedSrcX, movedSrcY);
    ExpectNear(movedSrcY, 872.35f, 0.05f, "mdg137 moved source y");
    RemapFlashC5Contain(moved, kSceneW, kSceneH, kPoolW, kPoolH);
    float movedDstX = 0.f, movedDstY = 0.f;
    ProjectFlashInt16ToPixel(moved, kXFirst, kY, 0.f, 0.f, kPoolW, kPoolH, movedDstX, movedDstY);
    ExpectNear(movedDstX, 393.09f, 0.05f, "mdg137 moved pool x");
    ExpectNear(movedDstY, 245.80f, 0.05f, "mdg137 moved pool y");
    assert(std::fabs(movedDstY - dstY) > 50.f);

    // 1280x720 main-menu canvas maps onto itself.
    float mainC[16];
    MdgMain74FlashC5(mainC);
    float mainMapped[16];
    std::memcpy(mainMapped, mainC, sizeof(mainMapped));
    RemapFlashC5Contain(mainMapped, 1280.f, 720.f, 1280.f, 720.f);
    for (int i = 0; i < 16; ++i) assert(std::fabs(mainMapped[i] - mainC[i]) < 1e-8f);

    std::puts("pause gfx: MDG 137 int16 placement, contain span, letterbox, scissor");
    return 0;
}
