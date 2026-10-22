#include "../locker_vr_loadout_layout.h"
#include <cmath>
#include <cstdio>

using namespace locker_loadout_layout;
static int fails = 0;
static void Expect(bool ok, const char* label) {
    if (!ok) { std::printf("FAIL %s\n", label); ++fails; }
}

static void TestEye(int eye, float yaw) {
    constexpr float fullW = 5264.f, fullH = 2684.f, halfW = fullW * .5f;
    const float a = yaw * 3.14159265f / 180.f;
    const uipointer::V forward{std::sin(a), 0, -std::cos(a)};
    const uipointer::V right{std::cos(a), 0, std::sin(a)};
    const uipointer::V up{0, 1, 0};
    uipointer::Plane plane{};
    Expect(uipointer::HeadPlane({}, forward, right, up, 1.4f, 1.f, 1.f, .42f, plane), "head plane");
    EyeProjector ep{};
    ep.pos = right * (eye ? .032f : -.032f);
    ep.fwd = forward; ep.right = right; ep.up = up;
    ep.tanHalfX = ep.tanHalfY = 1.f;
    const PanelProjectParams pp{&plane, eye, halfW, fullH, &ep};
    float left = 0, rightEdge = 0, ignored = 0;
    Expect(ProjectCanvasUvToScreen(pp, 0.f, .5f, left, ignored), "left edge projection");
    Expect(ProjectCanvasUvToScreen(pp, 1.f, .5f, rightEdge, ignored), "right edge projection");

    auto MakeLayout = [&](bool ready) {
        ComputeParams cp{};
        cp.eye = eye; cp.fullW = fullW; cp.fullH = fullH; cp.halfW = halfW;
        cp.dataReady = ready;
        const float halfPanel = ready ? kTileSpacingXPx * .5f + kIconHalfPx + 14.f
                                      : 15.f * (kGlyphW + 1) * cp.fontPx * .5f;
        Expect(ProjectAdjacentAnchor(pp, halfPanel, cp.anchorSx, cp.anchorSy), "project adjacent anchor");
        Layout layout{};
        Expect(ComputeLayout(cp, layout), "layout built");
        return layout;
    };
    const Layout layout = MakeLayout(true);
    const Layout loading = MakeLayout(false);
    const float comfortRight = (eye + 1) * halfW - halfW * .10f;
    Expect(layout.panel.x0 > rightEdge, "no native menu overlap");
    Expect(std::fabs(layout.panel.x0 - rightEdge - kNativeMenuGapPx) < .1f,
           "shared projected panel/native gap");
    Expect(layout.panel.x1 <= comfortRight && layout.panel.y0 >= kMarginPx &&
           layout.panel.y1 <= fullH - kMarginPx, "panel inside comfort bounds");
    Expect(loading.loadingLabel.x0 > rightEdge && loading.loadingLabel.x1 <= comfortRight,
           "loading label visible beside menu");
    Expect(layout.equipAll.x0 >= layout.panel.x0 && layout.equipAll.x1 <= layout.panel.x1 &&
           layout.equipAll.y0 >= layout.panel.y0 && layout.equipAll.y1 <= layout.panel.y1,
           "equip button inside panel");
    const float titleWidth = 10.f * (kGlyphW + 1) * 3.f; // VR LOADOUT
    Expect(layout.anchorSx - titleWidth * .5f >= layout.panel.x0 &&
           layout.anchorSx + titleWidth * .5f <= layout.panel.x1,
           "complete title inside panel");
    float topX = 0, topY = 0, bottomX = 0, bottomY = 0;
    ProjectCanvasUvToScreen(pp, .5f, 0.f, topX, topY);
    ProjectCanvasUvToScreen(pp, .5f, 1.f, bottomX, bottomY);
    auto CheckHit = [&](float sx, float sy, HitKind expected, int expectedTile) {
        const float u = (sx - left) / (rightEdge - left);
        const float v = (sy - topY) / (bottomY - topY);
        const uipointer::V target = uipointer::PointAtCanvasUv(plane, u, v);
        uipointer::Hit hit{};
        Expect(uipointer::Intersect(ep.pos, target - ep.pos, plane, 2, 2, hit, -.5f, 1.5f),
               "extended controller ray");
        float projectedX = 0, projectedY = 0;
        Expect(ProjectCanvasUvToScreen(pp, hit.u, hit.v, projectedX, projectedY), "project ray target");
        Expect(std::fabs(projectedX - sx) < .1f && std::fabs(projectedY - sy) < .1f,
               "laser/render alignment");
        int tile = -1;
        Expect(HitTestPointer(layout, projectedX, projectedY, tile) == expected && tile == expectedTile,
               "correct hit target");
    };
    for (int i = 0; i < 4; ++i) {
        Expect(layout.tiles[i].x0 >= layout.panel.x0 && layout.tiles[i].x1 <= layout.panel.x1 &&
               layout.tiles[i].y0 >= layout.panel.y0 && layout.tiles[i].y1 <= layout.panel.y1,
               "tile inside panel");
        CheckHit(layout.tileCenterX[i], layout.tileCenterY[i], HitKind::Tile, i);
    }
    float eqX = 0, eqY = 0;
    layout.equipAll.Center(eqX, eqY);
    CheckHit(eqX, eqY, HitKind::EquipAll, -1);
    std::printf("eye=%d yaw=%.0f nativeRight=%.1f panel=[%.1f,%.1f] equip=[%.1f,%.1f] loading=[%.1f,%.1f]\n",
                eye, yaw, rightEdge, layout.panel.x0, layout.panel.x1,
                layout.equipAll.x0, layout.equipAll.x1,
                loading.loadingLabel.x0, loading.loadingLabel.x1);
}

int main() {
    for (float yaw : {0.f, 25.f, -25.f})
        for (int eye = 0; eye < 2; ++eye) TestEye(eye, yaw);
    return fails ? 1 : 0;
}
