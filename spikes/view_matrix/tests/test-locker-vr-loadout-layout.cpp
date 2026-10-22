#include "../locker_vr_loadout_layout.h"
#include <cmath>
#include <cstdio>

static int g_fails = 0;
static void Expect(bool cond, const char* name) {
    if (cond) {
        std::printf("PASS %s\n", name);
        return;
    }
    std::printf("FAIL %s\n", name);
    ++g_fails;
}

int main() {
    using namespace locker_loadout_layout;

    ComputeParams cp{};
    cp.fullW = 3840.f;
    cp.fullH = 2160.f;
    cp.halfW = cp.fullW * .5f;
    cp.eye = 1;
    cp.anchorSx = 1200.f;
    cp.anchorSy = 900.f;
    cp.dataReady = true;
    cp.pending = true;
    cp.equipLabel = EquipLabelKind::EquipAll;

    cp.eye = 0;
    cp.anchorSx = 1770.f;
    cp.anchorSy = 1080.f;
    Layout base{};
    ComputeLayout(cp, base);
    const float equipCx0 = (base.equipAll.x0 + base.equipAll.x1) * .5f;

    cp.anchorSx = 1780.f;
    Layout shifted{};
    ComputeLayout(cp, shifted);
    const float equipCx1 = (shifted.equipAll.x0 + shifted.equipAll.x1) * .5f;
    Expect(std::fabs(shifted.anchorSx - base.anchorSx - 10.f) < 0.01f, "anchor clamp preserves +10px shift in band");
    Expect(std::fabs(equipCx1 - equipCx0 - 10.f) < 0.01f, "equip rect shifts +10px with anchor");

    Layout& clamped = shifted;
    for (int i = 0; i < 4; ++i) {
        float cx = 0, cy = 0;
        clamped.tiles[i].Center(cx, cy);
        Expect(clamped.tiles[i].Contains(clamped.tileCenterX[i], clamped.tileCenterY[i]),
               "tile center inside clickable rect");
        Expect(std::fabs(cx - clamped.tileCenterX[i]) < 1e-3f && std::fabs(cy - clamped.tileCenterY[i]) < 1e-3f,
               "tile center matches rect center");
    }
    float eqCx = 0, eqCy = 0;
    clamped.equipAll.Center(eqCx, eqCy);
    Expect(clamped.equipAll.Contains(eqCx, eqCy), "equip label center inside equip rect");

    Expect(clamped.panel.x0 >= kMarginPx - 1.f, "panel inside left eye min X");
    Expect(clamped.panel.x1 <= cp.halfW - kMarginPx + 1.f, "panel inside left eye max X");
    Expect(clamped.panel.y0 >= kMarginPx - 1.f && clamped.panel.y1 <= cp.fullH - kMarginPx + 1.f,
           "panel inside vertical viewport");

    int tile = -1;
    Expect(HitTestPointer(clamped, clamped.tileCenterX[0], clamped.tileCenterY[0], tile) == HitKind::Tile &&
               tile == 0,
           "pointer at tile center hits tile 0");
    Expect(HitTestPointer(clamped, eqCx, eqCy, tile) == HitKind::EquipAll, "pointer at equip center hits equip");

    cp.dataReady = false;
    Layout loading{};
    ComputeLayout(cp, loading);
    Expect(loading.loadingLabel.x1 - loading.loadingLabel.x0 > 0.f, "loading label has width");
    Expect(loading.equipAll.x1 <= loading.equipAll.x0, "loading layout has no equip rect");

    for (int eye : {0, 1}) {
        ComputeParams lp{};
        lp.eye = eye;
        lp.fullW = 3840.f;
        lp.fullH = 2160.f;
        lp.halfW = lp.fullW * .5f;
        lp.anchorSx = eye == 0 ? 400.f : 3400.f;
        lp.anchorSy = 1080.f;
        lp.dataReady = false;
        Layout loadEye{};
        ComputeLayout(lp, loadEye);
        const float eyeMinX = eye == 0 ? kMarginPx : lp.halfW + kMarginPx;
        const float eyeMaxX = eye == 0 ? lp.halfW - kMarginPx : lp.fullW - kMarginPx;
        Expect(loadEye.loadingLabel.x0 >= eyeMinX - 0.5f, "loading label min X inside eye");
        Expect(loadEye.loadingLabel.x1 <= eyeMaxX + 0.5f, "loading label max X inside eye");
        Expect(loadEye.loadingLabel.y0 >= kMarginPx - 0.5f, "loading label min Y inside viewport");
        Expect(loadEye.loadingLabel.y1 <= lp.fullH - kMarginPx + 0.5f, "loading label max Y inside viewport");
    }

    return g_fails == 0 ? 0 : 1;
}
