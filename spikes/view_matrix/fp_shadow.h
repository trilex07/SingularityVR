#pragma once
#include <cstddef>

// First-person shadow-caster draw classifier (isolated helper, no hooks).
//
// Root-cause research (see docs/REFERENCE_VR_IMPLEMENTATIONS.md and the
// shadow audit on branch feature/psvr2-polish-parallel):
//
// CONFIRMED (repo evidence, spikes/view_matrix/d3d9.cpp + skeletal_palette.h):
//   - The ONLY shadow-vs-color discriminator in code is the shader-layout
//     flag: skeletal::ReadLayout sets shadow = declares ProjectionMatrix AND
//     a DepthBias/InvMaxSubjectDepth constant (skeletal_palette.h:73),
//     cached per shader (d3d9.cpp Hook_CreateVS/Hook_SetVS) and used to
//     EXCLUDE draws from stereo splitting (StereoPairBody) and from every
//     first-person match rule (FgMatchWindow requires !shadow).
//   - First-person COLOR draws are keyed on vertex-count signatures
//     (FgMatchesSignature over g_fgStableVerts[]) inside the late
//     foreground pass (rtIsScene, after the depth+stencil Clear bracket).
//   - Previous 512x512-R32F-map detection / texture substitution was WRONG
//     and must not be revived (shadow_policy.h IsProjectedShadowMap is
//     log-only by design now).
// LIKELY (standard UE3 behaviour, no in-repo offsets):
//   - Dynamic skeletal shadows render in a separate light-space depth pass;
//     first-person arms/weapon are distinct USkeletalMeshComponent instances
//     whose CastShadow / bCastDynamicShadow flags could be cleared
//     per-component. NO confirmed component address/offset exists in-repo
//     (no Ghidra/marker hits) — do NOT poke flags until confirmed.
// SPECULATIVE:
//   - The per-eye-differing square/block visual is an FP-mesh depth/shadow
//     draw interacting with an eye-remapped pass; needs headset-verified
//     capture, not assumption.
//
// This predicate identifies "first-person arm/weapon shadow-caster draw
// ONLY" as a conjunction (Dishonored-VR ghost-sibling rule precedent:
// src/legacy/vr33/weapon_primitive_sibling_state.inc — a depth/shadow pass
// of the SAME mesh shares buffer/range identity with its color draw):
//   verts in current-frame FP signature set
//   AND shader-layout shadow flag set
//   AND offscreen render target (light-space pass, not the scene)
//   AND geometry-tuple sibling of a THIS-frame FP color draw
//     (same VB/offset/stride/IB/range) — mandatory so a world mesh that
//     merely collides on vertex count (run-215 floor-hole lesson) can never
//     match.
// It provably cannot match: normal FP color (requires scene+!shadow),
// world shadows (verts not in FP set or tuple mismatch), or the FP
// depth-prime draws that use the color shader (handled by the existing
// FgMatchWindow prime arm, not here).
//
// Pure logic: the main agent feeds it from Hook_DrawIndexedPrim AFTER the
// signature set is populated, behind a new INI switch defaulting to log-only.

namespace fpshadow {

struct DrawKey {
    unsigned verts = 0;
    unsigned first = 0; // start-vertex (FirstVertex for DIPI / StartVertex)
    unsigned prims = 0; // primitive count
    const void* vb = nullptr;
    const void* ib = nullptr;
    unsigned vbOffset = 0;
    unsigned stride = 0;
    int baseVertex = 0;
    unsigned minIndex = 0;
    unsigned startIndex = 0;
    bool layoutShadow = false; // skeletal::Layout.shadow for the bound VS
    bool rtIsScene = true; // render target is the scene backbuffer-size target
};

inline bool VertsInSignature(unsigned verts, const unsigned* sig, unsigned n) {
    if (!sig) return false;
    for (unsigned i = 0; i < n; ++i)
        if (sig[i] == verts) return true;
    return false;
}

inline bool SameGeometry(const DrawKey& a, const DrawKey& b) {
    return a.vb == b.vb && a.vbOffset == b.vbOffset && a.stride == b.stride && a.ib == b.ib &&
           a.baseVertex == b.baseVertex && a.minIndex == b.minIndex && a.verts == b.verts &&
           a.startIndex == b.startIndex && a.first == b.first && a.prims == b.prims;
}

// True only for the FP shadow-caster draw. colorDraws/colorN are the
// THIS-frame (or latched-stable) FP color draws already accepted by the
// production FgMatchesSignature + FgMatchWindow rules.
inline bool IsFpShadowCasterDraw(const DrawKey& d, const unsigned* sigVerts, unsigned sigN,
                                 const DrawKey* colorDraws, unsigned colorN) {
    if (!VertsInSignature(d.verts, sigVerts, sigN)) return false;
    if (!d.layoutShadow) return false;
    if (d.rtIsScene) return false;
    if (!colorDraws) return false;
    for (unsigned i = 0; i < colorN; ++i)
        if (SameGeometry(d, colorDraws[i])) return true;
    return false;
}

} // namespace fpshadow
