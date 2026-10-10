#pragma once
inline bool SuppressVrShadow(bool vrMode, bool shadowsEnabled, bool classifiedShadow) {
    return vrMode && !shadowsEnabled && classifiedShadow;
}
// Live render-target evidence identifies the projected arm shadow as a sampled
// square render-target texture. Common formats: R32F (114), A8R8G8B8 (21) RT.
// Accept common shadow map sizes (256-2048). Other R32F/A8R8G8B8 buffers may
// carry unrelated post-process data; the size+usage filter keeps it specific.
inline bool IsProjectedShadowMap(unsigned format,unsigned width,unsigned height) {
    // D3DFMT_R32F (114) - common for variance/PCF shadow maps
    if (format == 114u && width == height && width >= 256u && width <= 2048u) return true;
    // D3DFMT_R16F (113) - half-precision shadow maps
    if (format == 113u && width == height && width >= 256u && width <= 2048u) return true;
    return false;
}

// Render-target textures with DEPTH/RENDERTARGET usage. Shadow maps are often
// created as RT + depth-stencil (D24S8=75, D24X8=77, D16=80, D32=71).
inline bool IsProjectedShadowRenderTarget(unsigned format,unsigned width,unsigned height,
                                           unsigned usage) {
    // A8R8G8B8 (21) RT - observed on Steam (run 2026-10-09)
    if (format == 21u && width == height && width >= 256u && width <= 2048u && (usage & 0x1u) != 0)
        return true;
    // Depth-stencil formats used as shadow map RT
    if ((format == 75u || format == 77u || format == 80u || format == 71u) &&
        width == height && width >= 256u && width <= 2048u && (usage & 0x1u) != 0)
        return true;
    return false;
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
