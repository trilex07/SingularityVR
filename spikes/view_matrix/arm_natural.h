#pragma once

// Bounded natural-arm adapter.  This deliberately uses only the asset-derived
// TMD reference skeleton and the two section maps proven by the palette trace.
// It does not guess identity-index mappings for other meshes.
#include "singularity_bind_reference.h"

namespace armnatural {
using armbind::Mat;
using armik::V;

struct Cache {
    Mat bind[89]{};
    bool ready = false;
};

inline const Cache& TmdBind() {
    static Cache c;
    if (!c.ready) {
        constexpr unsigned n = sizeof(bindassets::tmd) / sizeof(bindassets::tmd[0]);
        c.ready = armbind::BuildReference(bindassets::tmd, n, c.bind);
    }
    return c;
}

inline V TransformPoint(const V p, const float m[4][4]) {
    return {
        p.x*m[0][0] + p.y*m[1][0] + p.z*m[2][0] + m[3][0],
        p.x*m[0][1] + p.y*m[1][1] + p.z*m[2][1] + m[3][1],
        p.x*m[0][2] + p.y*m[1][2] + p.z*m[2][2] + m[3][2]
    };
}

// Seed a new two-bone solve from the authored bend plane.  The reach axis is
// shoulder->wrist; using shoulder->elbow here would project the vector against
// itself and always select a world-space fallback.
inline bool PreferredBend(const V shoulder, const V elbow, const V wrist, V& out) {
    V reach;
    if (!armik::Unit(wrist - shoulder, reach)) return false;
    return armik::Unit((elbow - shoulder) - reach * armik::Dot(elbow - shoulder, reach), out);
}

// Build deltas against the exported reference pose.  meshDelta is the already
// established local->controller transform used by the existing hand path, so
// this keeps the coordinate convention identical to the weapon and fingers.
inline bool Build(bool right, const float meshDelta[4][4], armbind::Deltas& out) {
    const Cache& c = TmdBind();
    if (!c.ready || !meshDelta) return false;
    const unsigned shoulder = right ? 49u : 7u;
    const unsigned elbow    = right ? 50u : 8u;
    const unsigned wrist    = right ? 51u : 9u;
    const Mat& s = c.bind[shoulder], &e = c.bind[elbow], &w = c.bind[wrist];
    const V sp = armbind::Position(s), ep = armbind::Position(e), wp = armbind::Position(w);
    const float upper = armik::Length(ep - sp), fore = armik::Length(wp - ep);
    if (!(upper > 1e-3f && fore > 1e-3f)) return false;

    // A degenerate bind chain may still use the stable solver fallback, but
    // ordinary TMD arms retain their own authored elbow side.
    V bend;
    if (!PreferredBend(sp, ep, wp, bend)) bend = {0,0,1};
    armik::State state{};
    armik::Solution solved;
    const V target = TransformPoint(wp, meshDelta);
    if (!armik::Solve(sp, target, upper, fore, bend, state, solved)) return false;

    float oldUpper[4][4], oldFore[4][4], newUpper[4][4], newFore[4][4], targetWrist[4][4];
    // Keep the reference joint orientation as the baseline.  The target wrist
    // retains the controller's established orientation, including support grip.
    std::memcpy(oldUpper, s.m, sizeof(oldUpper));
    std::memcpy(oldFore, e.m, sizeof(oldFore));
    if (!armik::Frame(solved.shoulder, solved.elbow-solved.shoulder, state.bend, newUpper)) return false;
    if (!armik::Frame(solved.elbow, solved.wrist-solved.elbow, state.bend, newFore)) return false;
    skeletal::Multiply(targetWrist, w.m, meshDelta);
    Mat oldUpperMat, oldForeMat, newUpperMat, newForeMat, targetWristMat;
    std::memcpy(oldUpperMat.m, oldUpper, sizeof(oldUpper));
    std::memcpy(oldForeMat.m, oldFore, sizeof(oldFore));
    std::memcpy(newUpperMat.m, newUpper, sizeof(newUpper));
    std::memcpy(newForeMat.m, newFore, sizeof(newFore));
    std::memcpy(targetWristMat.m, targetWrist, sizeof(targetWrist));
    if (!armbind::Delta(oldUpperMat, newUpperMat, out.upper) ||
        !armbind::Delta(oldForeMat, newForeMat, out.fore) ||
        !armbind::Delta(w, targetWristMat, out.wrist)) return false;
    out.solution = solved;
    return true;
}

inline void Apply(float (*changed)[3][4], const float (*original)[3][4], unsigned slots,
                  bool right, const armbind::Deltas& d) {
    if (!changed || !original) return;
    // Exact TMD section 1 maps from the audited palette trace:
    // left  shoulder=slot0, forearm2=slot1, elbow=slot2;
    // right elbow=slot12, forearm=slot13, shoulder=slot14.
    const unsigned shoulderSlot = right ? 14u : 0u;
    const unsigned foreSlots[2] = { right ? 12u : 2u, right ? 13u : 1u };
    if (shoulderSlot < slots) skeletal::TransformBone(changed[shoulderSlot], original[shoulderSlot], d.upper.m);
    for (unsigned i=0;i<2;++i) if (foreSlots[i] < slots)
        skeletal::TransformBone(changed[foreSlots[i]], original[foreSlots[i]], d.fore.m);
}

inline void ApplySection0(float (*changed)[3][4], const float (*original)[3][4], unsigned slots,
                          bool right, const armbind::Deltas& d) {
    if (!changed || !original) return;
    // Exact TMD section 0 maps: left wrist=slot1, left forearm=slot18,
    // right wrist=slot19.  The arm upper/elbow are authored in section 1.
    const unsigned wristSlot = right ? 19u : 1u;
    if (wristSlot < slots) skeletal::TransformBone(changed[wristSlot], original[wristSlot], d.wrist.m);
    const unsigned foreSlot = right ? 0xffffffffu : 18u;
    if (foreSlot < slots) skeletal::TransformBone(changed[foreSlot], original[foreSlot], d.fore.m);
}
}
