#pragma once
inline bool SuppressVrShadow(bool vrMode, bool shadowsEnabled, bool classifiedShadow) {
    return vrMode && !shadowsEnabled && classifiedShadow;
}
// Live render-target evidence identifies the projected arm shadow as a sampled 512x512 R32F map.
// Keep this deliberately exact: other R32F buffers may carry unrelated post-process data.
inline bool IsProjectedShadowMap(unsigned format,unsigned width,unsigned height) {
    return format==114u && width==512u && height==512u; // D3DFMT_R32F
}

// A first-person shadow caster is identified at the draw seam by the already-latched
// arms/weapon vertex fingerprints.  This policy never treats a texture as a shadow and
// never changes the normal colour/depth pass.
inline bool SuppressFirstPersonShadowCaster(bool vrMode, bool shadowPass, bool shadowsEnabled,
                                            unsigned vertices, unsigned gunVertices,
                                            unsigned armVertices) {
    if (!vrMode || !shadowPass || shadowsEnabled || !vertices) return false;
    return vertices == gunVertices || vertices == armVertices;
}
