#pragma once
#include <cmath>
#include <cstdint>
namespace laserstyle {
inline int Clamp(int value,int low,int high){return value<low?low:value>high?high:value;}
inline std::uint32_t Brightness(std::uint32_t colour,int pct){
    return (colour&0x00ffffffu)|(((colour>>24)*Clamp(pct,10,100)/100u)<<24);
}
// 0 off, 1 dot only, 2 thin beam and dot. Legacy LaserSight stays boolean.
inline int Mode(bool enabled,bool dotOnly){return !enabled?0:dotOnly?1:2;}
inline int Next(int mode,int direction){return (mode+(direction<0?2:1))%3;}
inline float HalfWidth(float eyeWidth){return std::fmax(.3f,eyeWidth*.0003f);}
inline float DotRadius(float eyeWidth){return std::fmax(1.25f,eyeWidth*.0012f);}
inline float Shimmer(std::uint32_t ms){return .9f+.1f*std::sin((ms%10000)*.008f);}
}
