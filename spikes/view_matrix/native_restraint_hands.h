#pragma once
#include <cstring>

namespace restraint {
// Proven Kismet target: wp_rope.Skeletal.ch_inview_noTMD_rope_mesh.
// This is native animation ownership, NOT proof of a cinematic camera.
inline bool OwnsHands(bool enabled, bool livePawn, bool typedProperties,
                      const char* meshClass, const char* meshName,
                      const char* groupClass, const char* groupName,
                      const char* rootClass, const char* rootName, bool rootHasNoOuter) {
    return enabled && livePawn && typedProperties && rootHasNoOuter &&
        meshClass && meshName && groupClass && groupName && rootClass && rootName &&
        !std::strcmp(meshClass,"SkeletalMesh") &&
        !std::strcmp(meshName,"ch_inview_noTMD_rope_mesh") &&
        !std::strcmp(groupClass,"Package") && !std::strcmp(groupName,"Skeletal") &&
        !std::strcmp(rootClass,"Package") && !std::strcmp(rootName,"wp_rope");
}
}
