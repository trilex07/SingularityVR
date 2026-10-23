#pragma once
#include <cstdint>
#include <cmath>
namespace twohandspread {
// UE3 FRotator wraps at 65536. Scale only the native random angular delta;
// preserve the native central aim (including its recoil) and roll.
inline int Reduced(int centre,int native,int percent){
    if(percent>=100||percent<0)return native;
    const std::uint32_t d=(std::uint32_t(native)-std::uint32_t(centre))&65535u;
    const int delta=d>=32768u?int(d)-65536:int(d);
    const int scaled=int(std::lround(double(delta)*percent/100.0));
    return int((std::uint32_t(centre)+std::uint32_t(scaled))&65535u);
}
inline bool Apply(int* result,int pitch,int yaw,int percent,bool localFire,bool support){
    if(!result||!localFire||!support||percent<0||percent>=100)return false;
    result[0]=Reduced(pitch,result[0],percent);
    result[1]=Reduced(yaw,result[1],percent);
    return true;
}
}
