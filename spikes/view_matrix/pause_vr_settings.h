#pragma once

// Geometry for the synthetic last row exposed by the native menu overlay.
// Coordinates are normalized logical GFx canvas coordinates (top-left origin), so
// the hit test is independent of SBS/window/render resolution.
// Class 2 stays at V 0.70-0.80. Class 3 is one band higher, V 0.60-0.70.
namespace pausevrsettings {
constexpr float kU0 = 0.25f;
constexpr float kU1 = 0.75f;
constexpr float kMainV0 = 0.70f;
constexpr float kMainV1 = 0.80f;
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
