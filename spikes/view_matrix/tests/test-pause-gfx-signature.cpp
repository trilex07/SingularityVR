#include <cassert>
#include <cstdio>
#include "../pause_gfx_signature.h"

int main() {
    using namespace pausegfx;
    pausegfx::ResetLearnedFlashPs();
    assert(FlashStrideSlot(12) == 0);
    assert(FlashStrideSlot(8) == 1);
    assert(FlashStrideSlot(32) < 0);
    float c[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 100, 200, 0, 1};
    ScaleGfxRowConstantsForViewport(c, 5264.f, 2684.f, 1280.f, 720.f);
    assert(c[0] > 0.2f && c[0] < 0.3f);
    assert(c[12] > 20.f && c[12] < 30.f);
    RECT r{0, 0, 5264, 2684};
    ScaleScissorRect(r, 1280.f / 5264.f, 720.f / 2684.f);
    assert(r.right == 1280 && r.bottom == 720);
    std::puts("pause gfx signature: stride slots, viewport scale, scissor scale");
    return 0;
}
