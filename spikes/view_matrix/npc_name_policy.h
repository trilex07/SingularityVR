#pragma once
#include <cmath>
namespace npcname {
// Undefined GFx numeric variables read as zero: never create phantom clips.
inline bool ShouldScale(float x,float y,int percent) {
    return percent>=40 && percent<100 && std::isfinite(x) && std::isfinite(y) &&
        std::fabs(x-100.f)<0.01f && std::fabs(y-100.f)<0.01f;
}
}
