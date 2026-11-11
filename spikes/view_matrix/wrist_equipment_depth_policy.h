#pragma once
#include <cstdint>

namespace wristhud {
// The game clears world depth at the first-person boundary. A Present-time
// depth surface thereafter cannot prove occlusion by walls. A preview never
// authorizes health suppression or production use.
struct DepthEvidence {
    bool sameSceneSurface=false;
    bool bothEyes=false;
    bool worldDepthSurvived=false;
    bool zMappingValidated=false;
};
inline bool ProductionDepth(const DepthEvidence& e){
    return e.sameSceneSurface&&e.bothEyes&&e.worldDepthSurvived&&e.zMappingValidated;
}
inline bool MayDraw(bool enabled,bool preview,const DepthEvidence& e){
    return enabled&&(preview||ProductionDepth(e));
}
inline bool MaySuppress(bool bothEyesDrawn,bool validHealth,bool preview,const DepthEvidence& e){
    return bothEyesDrawn&&validHealth&&!preview&&ProductionDepth(e);
}
}
