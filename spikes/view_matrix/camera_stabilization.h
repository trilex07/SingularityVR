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

// Clears residual flat-game camera roll, head bobbing, and camera shake from
// the camera POV rotation so that physical OpenXR HMD tracking is 100% stable in VR.
inline bool StabilizeVrCameraRotation(int32_t wantPitch, int32_t wantYaw,
                                      int32_t povPitch, int32_t povYaw, int32_t povRoll,
                                      int32_t& outPitch, int32_t& outYaw, int32_t& outRoll,
                                      bool suppressShake = true) {
    outPitch = suppressShake ? wantPitch : povPitch;
    outYaw   = suppressShake ? wantYaw : povYaw;
    outRoll  = 0; // Engine roll is always zeroed; OpenXR HMD tracking supplies physical roll
    return true;
}

