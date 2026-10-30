#pragma once
#include "native_rifle_fx_policy.h"
namespace muzzleabi {
using SetBase=void (__thiscall*)(void*,void*,hipknife::V,int,void*,std::uint32_t,std::uint32_t);
inline void Detach(SetBase method,void* emitter){method(emitter,nullptr,{0,0,1},1,nullptr,0,0);}
// Full 431-byte native +130 body: two FVector references followed by nine
// unchanged 32-bit arguments, callee ret 2Ch. The second vector is converted
// by FVector::Rotation, NOT interpreted as an FRotator.
using Method=void (__thiscall*)(void*,const hipknife::V*,const hipknife::V*,
    std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,
    std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t);
inline void Forward(Method fn,void* self,const hipknife::V* position,const hipknife::V* direction,
    std::uint32_t a3,std::uint32_t a4,std::uint32_t a5,std::uint32_t a6,std::uint32_t a7,
    std::uint32_t a8,std::uint32_t a9,std::uint32_t a10,std::uint32_t a11){
    fn(self,position,direction,a3,a4,a5,a6,a7,a8,a9,a10,a11);
}
}
