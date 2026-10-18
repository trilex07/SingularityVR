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

// Presentation-only main/frontend tuning. Never re-scale the captured producer
// RT: that would shrink twice and separate the native labels from pointer UVs.
// 100 and distance=0 are exact opt-out; pause/boot/mod panel stay accepted.
inline float FrontMenuInset(int menuClass,bool modPanel,float inset,int percent){
    if(menuClass!=2||modPanel)return inset;
    const int pct=percent<70?70:(percent>100?100:percent);
    return inset*(pct/100.f);
}
inline float FrontMenuDistance(int menuClass,bool modPanel,int sharedMm,int frontMm){
    if(menuClass!=2||modPanel||frontMm<=0)return sharedMm/1000.f;
    const int mm=frontMm<600?600:(frontMm>3000?3000:frontMm);
    return mm/1000.f;
}

// Direct contextual canvases and their input ray must use the same comfort
// depth. Keep pause, RTT movies and the mod settings panel unchanged.
inline float ContextMenuDistance(int menuClass,bool modPanel,bool projectDirect,
                                 float existing,int frontMm){
    if(menuClass!=4||modPanel||!projectDirect||frontMm<=0)return existing;
    return FrontMenuDistance(2,false,0,frontMm);
}
