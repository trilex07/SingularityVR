#pragma once
// Class-1 backbuffer framing only.
//
// The preserved headset log recorded `ini: MenuInsetPct=100` and then
// `menu safe: class=1 scale=0.84`. That 0.84 is the shared menuSafe clamp,
// and the IIS title still sat on the edge of each eye. The source safe-area
// default is 70. Physical follow-up asks for more centring: class 1 now caps
// at 45, retaining tighter INIs. Classes 2 and 3 stay on the 0.84 limit. Menu distance and
// ndcShift are not decided here.

inline float ClassBackbufferScale(int menuClass, bool menuSafe, int insetPct) {
    float k = insetPct / 100.0f;
    if (menuClass == 1) {
        if (k > 0.45f) k = 0.45f;
        return k;
    }
    if (menuSafe && k > 0.84f) k = 0.84f;
    return k;
}

// Main-menu placement only: +8% of eye-image height, expressed in clip Y.
// Shared by native backbuffer HUD and tracking-space provider/pointer plane.
inline float MenuPlacementClipY(int menuClass) { return menuClass == 2 ? -0.16f : 0.f; }
