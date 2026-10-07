#pragma once
inline bool SuppressVrShadow(bool vrMode, bool shadowsEnabled, bool classifiedShadow) {
    return vrMode && !shadowsEnabled && classifiedShadow;
}
// Live render-target evidence identifies the projected arm shadow as a sampled 512x512 R32F map.
// Keep this deliberately exact: other R32F buffers may carry unrelated post-process data.
inline bool IsProjectedShadowMap(unsigned format,unsigned width,unsigned height) {
    return format==114u && width==512u && height==512u; // D3DFMT_R32F
}
