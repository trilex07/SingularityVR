#pragma once
// Shared WeaponLocker VR loadout panel geometry: one layout path for draw + XR hit-test.
#include "ui_pointer.h"
#include <cmath>
#include <cstring>

namespace locker_loadout_layout {

constexpr float kAnchorU = 0.93f;
constexpr float kAnchorV = 0.50f;
constexpr float kIconHalfPx = 42.f;
constexpr float kTileSpacingXPx = 110.f;
constexpr float kTileSpacingYPx = 105.f;
constexpr float kMarginPx = 28.f;
constexpr int kGlyphW = 5;
constexpr int kGlyphH = 7;

struct ScreenRect {
    float x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    bool Contains(float x, float y) const { return x >= x0 && x <= x1 && y >= y0 && y <= y1; }
    void Center(float& cx, float& cy) const {
        cx = (x0 + x1) * .5f;
        cy = (y0 + y1) * .5f;
    }
};

struct EyeProjector {
    uipointer::V pos{}, fwd{}, right{}, up{};
    float tanHalfX = 0, tanHalfY = 0;
};

struct PanelProjectParams {
    const uipointer::Plane* plane = nullptr;
    int eye = 0;
    float halfW = 0;
    float fullH = 0;
    const EyeProjector* eyeProj = nullptr;
};

inline bool ProjectCanvasUvToScreen(const PanelProjectParams& p, float canvasU, float canvasV, float& sx,
                                    float& sy) {
    if (!p.plane || !p.eyeProj || p.halfW <= 0 || p.fullH <= 0) return false;
    const uipointer::V point = uipointer::PointAtCanvasUv(*p.plane, canvasU, canvasV);
    uipointer::ScreenPoint sp{};
    const EyeProjector& e = *p.eyeProj;
    if (!uipointer::ProjectPoint(point, e.pos, e.fwd, e.right, e.up, e.tanHalfX, e.tanHalfY, sp)) return false;
    sx = (p.eye + sp.u) * p.halfW;
    sy = sp.v * p.fullH;
    return std::isfinite(sx) && std::isfinite(sy);
}

inline void ClampEyePanel(int eye, float fullW, float fullH, float halfW, float& cx, float& cy, float halfPanelW,
                          float halfPanelH, float margin) {
    const float eyeMinX = eye == 0 ? margin : halfW + margin;
    const float eyeMaxX = eye == 0 ? halfW - margin : fullW - margin;
    const float minY = margin + halfPanelH;
    const float maxY = fullH - margin - halfPanelH;
    const float preferredCx = eyeMaxX - halfPanelW - margin * .5f;
    if (cx < preferredCx) cx = preferredCx;
    if (cx - halfPanelW < eyeMinX) cx = eyeMinX + halfPanelW;
    if (cx + halfPanelW > eyeMaxX) cx = eyeMaxX - halfPanelW;
    if (cy - halfPanelH < minY - halfPanelH) cy = minY;
    if (cy + halfPanelH > maxY + halfPanelH) cy = maxY;
    if (cy < margin + 12.f) cy = margin + 12.f;
    if (cy > fullH - margin - 12.f) cy = fullH - margin - 12.f;
}

enum class EquipLabelKind { AllOwned, EquipAll, Queued };

struct ComputeParams {
    int eye = 0;
    float fullW = 0, fullH = 0, halfW = 0;
    float anchorSx = 0, anchorSy = 0;
    float iconHalf = kIconHalfPx;
    float spacingX = kTileSpacingXPx;
    float spacingY = kTileSpacingYPx;
    int fontPx = 3;
    bool dataReady = false;
    bool pending = false;
    EquipLabelKind equipLabel = EquipLabelKind::AllOwned;
};

struct Layout {
    bool valid = false;
    float anchorSx = 0, anchorSy = 0;
    ScreenRect panel{};
    ScreenRect tiles[4]{};
    float tileCenterX[4]{}, tileCenterY[4]{};
    ScreenRect equipAll{};
    ScreenRect loadingLabel{};
};

inline int EquipLabelCharCount(EquipLabelKind kind) {
    switch (kind) {
    case EquipLabelKind::Queued: return 6;
    case EquipLabelKind::EquipAll: return 9;
    default: return 9; // "ALL OWNED"
    }
}

inline bool ComputeLayout(ComputeParams p, Layout& out) {
    out = Layout{};
    if (p.fullW <= 0 || p.fullH <= 0 || p.halfW <= 0) return false;
    out.valid = true;

    if (!p.dataReady) {
        const int px = p.fontPx;
        const int lw = 15 * (kGlyphW + 1) * px;
        const int lh = kGlyphH * px;
        const float halfLabelW = lw * .5f;
        const float halfLabelH = lh * .5f;
        ClampEyePanel(p.eye, p.fullW, p.fullH, p.halfW, p.anchorSx, p.anchorSy, halfLabelW, halfLabelH, kMarginPx);
        out.anchorSx = p.anchorSx;
        out.anchorSy = p.anchorSy;
        const float lx = p.anchorSx - halfLabelW;
        const float ly = p.anchorSy - halfLabelH;
        out.loadingLabel = {lx, ly, lx + (float)lw, ly + (float)lh};
        return true;
    }

    const float panelHalfW = p.spacingX * .5f + p.iconHalf + 14.f;
    const float panelHalfH = p.spacingY * .5f + p.iconHalf + 48.f;
    ClampEyePanel(p.eye, p.fullW, p.fullH, p.halfW, p.anchorSx, p.anchorSy, panelHalfW, panelHalfH, kMarginPx);
    out.anchorSx = p.anchorSx;
    out.anchorSy = p.anchorSy;

    out.panel = {p.anchorSx - panelHalfW, p.anchorSy - panelHalfH, p.anchorSx + panelHalfW,
                 p.anchorSy + panelHalfH};

    const float tileOffX[4] = {-p.spacingX * .5f, p.spacingX * .5f, -p.spacingX * .5f, p.spacingX * .5f};
    const float tileOffY[4] = {-p.spacingY * .5f, -p.spacingY * .5f, p.spacingY * .5f, p.spacingY * .5f};
    for (int ti = 0; ti < 4; ++ti) {
        const float cx = p.anchorSx + tileOffX[ti];
        const float cy = p.anchorSy + tileOffY[ti] + 6.f;
        out.tileCenterX[ti] = cx;
        out.tileCenterY[ti] = cy;
        out.tiles[ti] = {cx - p.iconHalf, cy - p.iconHalf, cx + p.iconHalf, cy + p.iconHalf};
    }

    const float bx = p.anchorSx;
    const float by = p.anchorSy + panelHalfH - 22.f;
    const int labelLen = EquipLabelCharCount(p.equipLabel);
    const int px = p.fontPx;
    const int bw = labelLen * (kGlyphW + 1) * px;
    const int bh = kGlyphH * px + 4;
    out.equipAll = {bx - bw * .5f - 4.f, by - 2.f, bx + bw * .5f + 4.f, by + (float)bh};
    return true;
}

enum class HitKind { None, EquipAll, Tile };

inline HitKind HitTestPointer(const Layout& layout, float px, float py, int& tileIndex) {
    tileIndex = -1;
    if (!layout.valid) return HitKind::None;
    if (layout.equipAll.Contains(px, py)) return HitKind::EquipAll;
    for (int i = 0; i < 4; ++i) {
        if (layout.tiles[i].Contains(px, py)) {
            tileIndex = i;
            return HitKind::Tile;
        }
    }
    return HitKind::None;
}

} // namespace locker_loadout_layout
