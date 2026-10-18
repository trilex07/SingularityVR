#pragma once

namespace contextcursor {

// Historical candidate signature, FALSIFIED by the class-4 runtime census:
// DrawIndexedPrimitiveUP, triangle list, 4 primitives, 8 vertices, 72-byte
// vertices. It appeared in class 2, not Dialog. Retained for diagnostic
// comparison only; it must never authorize suppression.
inline bool MatchesObservedDraw(bool vrMode, bool vrUiEnabled, bool modalMovie,
                               bool directCanvas, bool sceneTarget,
                               unsigned primitiveType, unsigned primitiveCount,
                               unsigned vertexCount, unsigned stride) {
    return vrMode && vrUiEnabled && modalMovie && directCanvas && sceneTarget &&
           primitiveType == 4u && primitiveCount == 4u && vertexCount == 8u &&
           stride == 72u;
}

inline bool ShouldSuppress(bool, bool, bool, bool, bool, bool,
                           unsigned, unsigned, unsigned, unsigned) {
    return false;
}

} // namespace contextcursor
