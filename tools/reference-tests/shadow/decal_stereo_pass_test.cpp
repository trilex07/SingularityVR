// Offline tests for deferred-decal stereo PS constant remap.
#include <cstdio>
#include <cmath>

#include "../../../spikes/view_matrix/decal_stereo_pass.h"

static int g_fail = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) {
        std::fprintf(stderr, "FAIL: %s\n", msg);
        ++g_fail;
    }
}

int main() {
    using singularity::decal::IsProjectionTargetFormat;
    using singularity::decal::IsSceneSizedTarget;
    using singularity::decal::RemapScreenPositionScaleBiasForEye;

    Check(IsProjectionTargetFormat(34u), "G16R16");
    Check(IsProjectionTargetFormat(112u), "G16R16F");
    Check(!IsProjectionTargetFormat(22u), "not G16R16");

    Check(IsSceneSizedTarget(2560, 1440, 2560, 1440), "scene sized");
    Check(!IsSceneSizedTarget(512, 512, 2560, 1440), "not scene sized");

    float c[4] = {0.5f, -0.5f, 0.5002f, 0.5001f};
    float left[4];
    float right[4];
    for (int i = 0; i < 4; ++i) {
        left[i] = c[i];
        right[i] = c[i];
    }
    RemapScreenPositionScaleBiasForEye(left, 0);
    RemapScreenPositionScaleBiasForEye(right, 1);

    Check(std::fabs(left[0] - 0.25f) < 1e-5f, "left scale x");
    Check(std::fabs(left[3] - 0.25005f) < 1e-4f, "left bias x");
    Check(std::fabs(right[0] - 0.25f) < 1e-5f, "right scale x");
    Check(std::fabs(right[3] - 0.75005f) < 1e-4f, "right bias x");
    Check(left[1] == c[1] && left[2] == c[2], "Y untouched left");
    Check(right[1] == c[1] && right[2] == c[2], "Y untouched right");

    std::fprintf(stderr, g_fail ? "decal_stereo_pass_test: %d failure(s)\n" : "decal_stereo_pass_test: PASS\n",
                 g_fail);
    return g_fail ? 1 : 0;
}
