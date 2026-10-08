#pragma once

// Geometry for the synthetic last row exposed by the native pause-menu overlay.
// Coordinates are normalized logical GFx canvas coordinates (top-left origin), so
// the hit test is independent of SBS/window/render resolution.
namespace pausevrsettings {
constexpr float kU0 = 0.25f;
constexpr float kU1 = 0.75f;
constexpr float kV0 = 0.86f;
constexpr float kV1 = 0.95f;

inline bool Hit(float u, float v) {
    return u >= kU0 && u <= kU1 && v >= kV0 && v <= kV1;
}
}
