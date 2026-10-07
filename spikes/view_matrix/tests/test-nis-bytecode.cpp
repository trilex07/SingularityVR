// Validates embedded NIS bytecode headers (offline, no D3D device needed).
// Checks DXBC magic, non-trivial size, and declared-size consistency.
#include <cstdio>
#include "../nis/NIS_Scaler_CS.h"
#include "../nis/NIS_Sharpen_CS.h"

int main() {
    int fail = 0;
    auto check = [&](const unsigned char* b, unsigned int n, const char* name) {
        if (n < 16) { std::printf("FAIL %s: too small (%u)\n", name, n); ++fail; return; }
        unsigned int magic = (unsigned int)b[0] | ((unsigned int)b[1] << 8) |
                             ((unsigned int)b[2] << 16) | ((unsigned int)b[3] << 24);
        if (magic != 0x43425844u) { std::printf("FAIL %s: bad DXBC magic %08X\n", name, magic); ++fail; return; }
        std::printf("ok %s: %u bytes, DXBC magic\n", name, n);
    };
    check(kNisScalerCS, kNisScalerCS_Size, "scaler");
    check(kNisSharpenCS, kNisSharpenCS_Size, "sharpen");
    if (sizeof(kNisScalerCS) != kNisScalerCS_Size) { std::printf("FAIL scaler size mismatch\n"); ++fail; }
    if (sizeof(kNisSharpenCS) != kNisSharpenCS_Size) { std::printf("FAIL sharpen size mismatch\n"); ++fail; }
    // The two shaders must differ (scaler != sharpen-only).
    if (kNisScalerCS_Size == kNisSharpenCS_Size) { std::printf("FAIL identical sizes\n"); ++fail; }
    std::printf(fail ? "VALIDATION FAILED\n" : "VALIDATION PASSED\n");
    return fail ? 1 : 0;
}
