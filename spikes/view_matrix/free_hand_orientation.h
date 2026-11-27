#pragma once
#include "hand_anchor_math.h"

namespace freehand {
// Row-vector axes. The index side of a hand points toward the thumb.
inline bool Basis(armik::V forward, armik::V thumbSide, armbind::Mat& frame) {
    armik::V f, u, n;
    if (!armik::Unit(forward, f) || !armik::Unit(armik::Cross(f, thumbSide), n))
        return false;
    u = armik::Cross(n, f);
    const armik::V rows[] = {f, u, n};
    frame = {};
    for (unsigned i = 0; i < 3; ++i) {
        frame.m[i][0] = rows[i].x;
        frame.m[i][1] = rows[i].y;
        frame.m[i][2] = rows[i].z;
    }
    frame.m[3][3] = 1.f;
    return true;
}

inline bool Align(armik::V wrist, armik::V middle, armik::V index, armik::V pinky,
                  armik::V pointing, armik::V up, float scale, float out[4][4]) {
    armbind::Mat src, dst, inverse, delta;
    if (!std::isfinite(scale) || scale <= 0 ||
        !Basis(middle - wrist, index - pinky, src) || !Basis(pointing, up, dst) ||
        !skeletal::Inverse(inverse.m, src.m)) return false;
    skeletal::Multiply(delta.m, inverse.m, dst.m);
    for (unsigned i = 0; i < 3; ++i)
        for (unsigned j = 0; j < 3; ++j) delta.m[i][j] *= scale;
    if (!armbind::Finite(delta)) return false;
    std::memcpy(out, delta.m, sizeof(delta.m));
    return true;
}
}
