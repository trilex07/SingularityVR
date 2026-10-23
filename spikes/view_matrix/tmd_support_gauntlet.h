#pragma once
#include "support_grip.h"
#include <cstring>

// Presentation only. Native powers still use the existing support reservation
// and release-to-rearm input policy. No ownership/inventory/hands-state writes.
namespace tmdsupport {
inline bool Enabled(bool requested, bool shortAsset, bool cuffAssets,
                    bool context, bool independent, int isolate,
                    bool leftTracked, bool rightTracked) {
    return requested && shortAsset && cuffAssets && context && independent &&
        isolate==2 && leftTracked && rightTracked;
}
inline bool HideHardware(bool support, bool keepAssembly, bool palette, bool tracked) {
    return !palette || !tracked || (support && !keepAssembly);
}
// Same final camera-space target for glove, hardware, sleeve and verified
// socket attachments. A valid target never changes any authored bone matrix.
inline bool Target(float out[4][4], const float primary[4][4],
                   const float off[4][4], const float weapon[4][4],
                   bool support, bool authoredPair, long gunVerts) {
    std::memcpy(out,off,sizeof(float)*16);
    if(!support)return false;
    if(authoredPair) {std::memcpy(out,primary,sizeof(float)*16);return false;}
    float candidate[4][4];std::memcpy(candidate,off,sizeof(candidate));
    if(!ApplySupportGripTarget(candidate,weapon,gunVerts))return false;
    for(const auto& row:candidate)for(float v:row)if(!std::isfinite(v))return false;
    std::memcpy(out,candidate,sizeof(candidate));return true;
}
}
