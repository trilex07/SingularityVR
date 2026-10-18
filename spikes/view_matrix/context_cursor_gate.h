#pragma once

namespace contextcursor {

// Narrow signature from the live RvUIGFxMovie_Dialog MenuDrawTrace census:
// DrawIndexedPrimitiveUP, triangle list, 4 primitives, 8 vertices, 72-byte
// vertices. This is an experiment gate, not a general Scaleform classifier.
inline bool MatchesObservedDraw(bool vrMode, bool vrUiEnabled, bool modalMovie,
                               bool directCanvas, bool sceneTarget,
                               unsigned primitiveType, unsigned primitiveCount,
                               unsigned vertexCount, unsigned stride) {
    return vrMode && vrUiEnabled && modalMovie && directCanvas && sceneTarget &&
           primitiveType == 4u && primitiveCount == 4u && vertexCount == 8u &&
           stride == 72u;
}

inline bool ShouldSuppress(bool enabled, bool vrMode, bool vrUiEnabled,
                           bool modalMovie, bool directCanvas, bool sceneTarget,
                           unsigned primitiveType, unsigned primitiveCount,
                           unsigned vertexCount, unsigned stride) {
    return enabled && MatchesObservedDraw(vrMode, vrUiEnabled, modalMovie,
        directCanvas, sceneTarget, primitiveType, primitiveCount, vertexCount, stride);
}

} // namespace contextcursor
