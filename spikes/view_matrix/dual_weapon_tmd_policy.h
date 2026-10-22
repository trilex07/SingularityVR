#pragma once
#include <cstdint>

// Presentation/input ownership only. Never changes native HANDS state or powers.
namespace dualtmd {
inline bool Context(bool enabled, bool vr, bool gameplay, bool owned,
                    bool nativeHands, bool menu, bool paused, bool rightHanded) {
    return enabled && vr && gameplay && owned && !nativeHands && !menu && !paused && rightHanded;
}
inline bool Weapon(bool nativeWeapon, bool context, bool equipped, int hands) {
    return nativeWeapon || (context && equipped && hands == 1);
}
inline bool MeleeWindow(bool context, bool knifeHeld) { return !context || knifeHeld; }

// Release-to-rearm: releasing support with L2/Square still down must not fire.
struct PowerLatch {
    bool blocked=false;
    bool Update(bool enabled, bool reserved, bool down) {
        if(!enabled){blocked=false;return down;}
        if(reserved){blocked=down;return false;}
        if(!down)blocked=false;
        return down&&!blocked;
    }
};

// A bounded pre-hide census. Values are draw-signature evidence, NOT proof of
// resource identity or pixels. No allocation, clock calls or GPU readbacks.
struct Census {
    struct Row { std::uint32_t raw=0,passed=0,gunRide=0,nativeHands=0; } rows[3][2]{};
    unsigned frames=0, notes=0;
    static constexpr unsigned FrameBudget=5400, NoteBudget=12;
    bool Active()const{return frames<FrameBudget && notes<NoteBudget;}
    void Add(int hands,bool hardware,bool passed,bool gunRide,bool nativeHands) {
        if(!Active()||hands<0||hands>2)return;
        auto& r=rows[hands][hardware?1:0];
        if(!passed){++r.raw;if(nativeHands)++r.nativeHands;}
        else {++r.passed;if(gunRide)++r.gunRide;}
    }
    void Clear(){for(auto& group:rows)for(auto& r:group)r={};}
};
}
