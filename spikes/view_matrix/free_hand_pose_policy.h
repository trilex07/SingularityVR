#pragma once
namespace freehandpose {
struct State {
    bool wristAnchor=false, weaponHeld=false, support=false, knife=false, magazine=false;
    bool animated=false, nativeReady=false, gameplay=false, focused=false, tracked=false;
    bool stereo=false, paused=false, menu=false, cinematic=false;
};
inline bool OffhandFingers(const State& s) {
    return s.wristAnchor && s.weaponHeld && !s.support && !s.knife && !s.magazine &&
        !s.animated && s.nativeReady && s.gameplay && s.focused && s.tracked &&
        s.stereo && !s.paused && !s.menu && !s.cinematic;
}
} // namespace freehandpose
