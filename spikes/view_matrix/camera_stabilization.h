#pragma once
#include <cmath>

// Replace only residual engine POV translation. Pawn movement and eye height
// remain, while physical HMD motion is applied separately.
inline bool ComputeVrCameraStabilization(const float pov[3], const float pawn[3],
                                         float eyeHeight, float out[3]) {
    if (!pov || !pawn || !out || !std::isfinite(eyeHeight)) return false;
    for (int i=0;i<3;++i) if (!std::isfinite(pov[i]) || !std::isfinite(pawn[i])) return false;
    out[0] = pawn[0] - pov[0];
    out[1] = pawn[1] - pov[1];
    out[2] = pawn[2] + eyeHeight - pov[2];
    return std::isfinite(out[0]) && std::isfinite(out[1]) && std::isfinite(out[2]);
}
