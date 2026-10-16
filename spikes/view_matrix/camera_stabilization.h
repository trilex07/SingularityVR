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

// Eliminates engine camera roll oscillation from an orthonormal camera basis
// (right, up, forward) while preserving the exact forward look vector.
// In UE3 coordinates: +X is Forward, +Y is Right, +Z is Up.
// Zero roll means the right vector lies in the horizontal XY plane (right.z == 0).
inline bool CancelBasisRoll(const float rightIn[3], const float upIn[3], const float forwardIn[3],
                            float rightOut[3], float upOut[3], float forwardOut[3],
                            float R_out[3][3]) {
    if (!rightIn || !upIn || !forwardIn || !rightOut || !upOut || !forwardOut || !R_out) return false;

    const float fwdHorizLenSq = forwardIn[0] * forwardIn[0] + forwardIn[1] * forwardIn[1];
    if (fwdHorizLenSq < 1e-6f) return false;

    float rd[3] = { -forwardIn[1], forwardIn[0], 0.0f };
    const float rdLen = std::sqrt(rd[0] * rd[0] + rd[1] * rd[1]);
    if (rdLen < 1e-6f) return false;
    rd[0] /= rdLen;
    rd[1] /= rdLen;

    float ud[3] = {
        forwardIn[1] * rd[2] - forwardIn[2] * rd[1],
        forwardIn[2] * rd[0] - forwardIn[0] * rd[2],
        forwardIn[0] * rd[1] - forwardIn[1] * rd[0]
    };
    const float udLen = std::sqrt(ud[0] * ud[0] + ud[1] * ud[1] + ud[2] * ud[2]);
    if (udLen < 1e-6f) return false;
    ud[0] /= udLen;
    ud[1] /= udLen;
    ud[2] /= udLen;

    for (int i = 0; i < 3; ++i) {
        rightOut[i]   = rd[i];
        upOut[i]      = ud[i];
        forwardOut[i] = forwardIn[i];
    }

    for (int i = 0; i < 3; ++i) {
        for (int k = 0; k < 3; ++k) {
            R_out[i][k] = rd[i] * rightIn[k] + ud[i] * upIn[k] + forwardIn[i] * forwardIn[k];
        }
    }
    return true;
}

