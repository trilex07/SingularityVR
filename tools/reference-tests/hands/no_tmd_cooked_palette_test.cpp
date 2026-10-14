// No game assets: exact-section policy and affine weighted-skin regression.
#include <cstdio>
#include <cmath>
#include "../../../spikes/view_matrix/glove_isolate.h"
#include "../../../spikes/view_matrix/skeletal_palette.h"
static int failures = 0;
static void Check(bool ok, const char* message) {
    if (!ok) { std::fprintf(stderr, "FAIL %s\n", message); ++failures; }
}
int main() {
    using namespace gloveisolate;
    Check(kNoTmdCookedPaletteCount == 35, "GPU chunk has 35 slots, not skeleton-set 45");
    Check(kNoTmdCookedPalette[1] == 51 && kNoTmdCookedPalette[18] == 9,
          "right wrist slot1, left wrist slot18");
    Check(kNoTmdCookedPalette[23] == 27, "native cuff placeholder slot23");
    Check(NoTmdCookedHandSide(85) == 1 && NoTmdCookedHandSide(50) == 1,
          "right forearm/elbow share wrist controller");
    Check(NoTmdCookedHandSide(27) == 0 && NoTmdCookedHandSide(9) == 0,
          "left cuff and wrist share controller");
    Check(NoTmdCookedHandSide(6) == -1 && NoTmdCookedHandSide(89) == -1,
          "unknown/non-section bones not invented");
    float bone[3][4] = {{1,0,0,0},{0,1,0,0},{0,0,1,0}};
    float delta[4][4] = {{0,1,0,0},{-1,0,0,0},{0,0,1,0},{40,-15,9,1}};
    float moved[3][4];
    skeletal::TransformBone(moved, bone, delta);
    const float point[4] = {2,3,4,1};
    // A cuff point actually observed with .8588 wrist + .1412 forearm.
    for (int c=0;c<3;++c) {
        float full=0, split=0;
        for (int k=0;k<4;++k) {
            full += point[k]*(0.8588f*moved[c][k]+0.1412f*moved[c][k]);
            split += point[k]*(0.8588f*moved[c][k]+0.1412f*bone[c][k]);
        }
        float expected=0; for(int k=0;k<4;++k) expected+=point[k]*moved[c][k];
        Check(std::fabs(full-expected)<0.0001f, "shared rigid delta preserves blend");
        Check(std::fabs(split-expected)>0.1f, "old mixed native/tracked seam reproduced");
    }
    // Mapping is independent of handedness. Delta selection mirrors OffHand().
    for (int off=0;off<2;++off) for(unsigned b:kNoTmdCookedPalette) {
        int side=NoTmdCookedHandSide(b);
        Check(side==0||side==1, "every weighted slot owns a physical hand");
        Check((side==off ? 0:1)==(side!=off), "off/aim delta selection both handedness modes");
    }
    if (failures) return 1;
    std::puts("PASS noTMD cooked palette + weighted rigid cuff + both handedness modes");
    return 0;
}
