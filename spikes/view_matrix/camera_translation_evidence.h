#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>
namespace camera_translation {
struct Snapshot {
 float actor[3]{},pov[3]{},pawn[3]{},eyeOffset[3]{},eyeDelta[3]{};
 float eyeHeight=0;
 bool pawnValid=false,eyeHeightValid=false,leftEyeBase=false;
 uint32_t flags=0; // Caller records cinematic, authored arm animation and tracking state.
};
struct Parts {float povMinusActor[3],pawnEye[3],povMinusPawnEye[3],headMidpoint[3];};
inline bool Decompose(const Snapshot& s,Parts& p){
 for(int i=0;i<3;++i){if(!std::isfinite(s.actor[i])||!std::isfinite(s.pov[i])||!std::isfinite(s.eyeOffset[i])||!std::isfinite(s.eyeDelta[i])||(s.pawnValid&&!std::isfinite(s.pawn[i])))return false;}
 if(s.eyeHeightValid && !std::isfinite(s.eyeHeight))return false;
 for(int i=0;i<3;++i){
  p.povMinusActor[i]=s.pov[i]-s.actor[i];
  p.pawnEye[i]=s.pawn[i]+((i==2&&s.eyeHeightValid)?s.eyeHeight:0);
  p.povMinusPawnEye[i]=s.pawnValid?s.pov[i]-p.pawnEye[i]:0;
  p.headMidpoint[i]=s.eyeOffset[i]+(s.leftEyeBase?s.eyeDelta[i]*.5f:0);
 }
 return true;
}
// Sampling eligibility is pure and deterministic; disabled mode does no clock/device work.
struct Gate {
 unsigned samples=0;uint32_t last=0;bool haveLast=false;
 bool Offer(bool enabled,uint32_t now,bool gameplay){
  if(!enabled||!gameplay||samples>=240)return false;
  if(haveLast && uint32_t(now-last)<100)return false;
  last=now;haveLast=true;++samples;return true;
 }
};
}
