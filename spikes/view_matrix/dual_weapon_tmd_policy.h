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

// The native HANDS_TMD animation does not place the left wrist at the weapon's
// authored fore-end. A Grip can be accepted while the physical hand is still
// behind/below the gun; using that inter-controller vector as barrel direction
// folds the weapon toward the body. Keep the right controller's measured aim in
// this one state. HANDS_WEAPON retains the accepted two-hand direction policy.
inline bool UseSupportAimVector(bool twoHand, bool dualContext, int hands) {
    return twoHand && !(dualContext && hands == 1);
}

// The native TMD mesh has three exact material sections. In dual presentation
// its bones are independently retargeted to the tracked wrists; it must not
// also inherit the camera-space ride selected for the right-hand gun.
inline bool NativeTmdSection(unsigned verts, unsigned first, unsigned triangles) {
    return ((verts == 7444u || verts == 2626u) && first == 0u && triangles == 4402u) ||
           ((verts == 7444u || verts == 4818u || verts == 4781u) && first == 13206u && triangles == 7704u) ||
           ((verts == 7444u || verts == 37u) && first == 36318u && triangles == 17u);
}
inline bool IndependentTmdOwnsPose(bool dualContext, bool independentHands,
                                   bool scene, bool shadow, bool foreground,
                                   unsigned verts, unsigned first, unsigned triangles) {
    return dualContext && independentHands && scene && !shadow && foreground &&
           NativeTmdSection(verts, first, triangles);
}

// The authored two-hand pair belongs to the weapon-hands animation. In
// HANDS_TMD the same material-0 glove starts in the TMD pose instead; a rigid
// copy of the gun-hand delta cannot put it on the weapon fore-end. The pistol
// has no fore-end and retains its accepted close-support policy.
inline bool UseWeaponAuthoredPair(bool authored, bool dualContext,
                                  int hands, long gunVerts) {
    return authored && !(dualContext && hands == 1 && gunVerts != 5799);
}

// The material-1 draw is the original 6527-face device plus 1177 arm faces.
// A missed foreground latch must not submit all 7704 faces in focused dual
// gameplay merely because the replacement/palette route was unavailable.
inline bool GuardMixedTmdSection(bool feature, bool dualContext, bool gameplay,
                                 bool scene, bool shadow, bool animated,
                                 bool originalShader, unsigned minIndex,
                                 unsigned verts, unsigned first, unsigned triangles) {
    return feature && dualContext && gameplay && scene && !shadow && !animated &&
           originalShader && minIndex == 2626u &&
           (verts == 4818u || verts == 4781u) &&
           first == 13206u && triangles == 7704u;
}

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
