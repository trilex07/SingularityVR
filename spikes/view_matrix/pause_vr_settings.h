#pragma once

// Geometry for the synthetic last row exposed by the native menu overlay.
// Coordinates are normalized logical GFx canvas coordinates (top-left origin), so
// the hit test is independent of SBS/window/render resolution.
// Class 2 raised by 0.04 canvas V (~3.4% of eye height at inset 0.84).
// Class 3 stays at its physically accepted V 0.60-0.70.
namespace pausevrsettings {
constexpr float kU0 = 0.25f;
constexpr float kU1 = 0.75f;
constexpr float kMainV0 = 0.66f;
constexpr float kMainV1 = 0.76f;
constexpr float kPauseV0 = 0.60f;
constexpr float kPauseV1 = 0.70f;

inline bool Row(int menuClass, float& v0, float& v1) {
    if (menuClass == 3) { v0 = kPauseV0; v1 = kPauseV1; return true; }
    if (menuClass == 2) { v0 = kMainV0; v1 = kMainV1; return true; }
    return false;
}

inline bool Hit(int menuClass, float u, float v) {
    float v0 = 0.0f, v1 = 0.0f;
    if (!Row(menuClass, v0, v1)) return false;
    return u >= kU0 && u <= kU1 && v >= v0 && v <= v1;
}
}
