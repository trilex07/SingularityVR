#pragma once
#include <cstddef>
#include <cstdint>
namespace inventorypickup {
// Owned native evidence: RvPlayerPawnSP vtable +D70, four stack arguments,
// ret16. Argument four is read only at +1A4; nonzero skips BOTH TossInventory
// calls (+1D4/+205), not creation, selection, slot notification or ammo setup.
// The game's own restore caller already supplies (archetype,0,0,1).
constexpr unsigned Slot=0xd70,Bytes=748;
constexpr uint64_t Fingerprint=0xCC78CCB4EE3651A2ull;
inline uint64_t Hash(const unsigned char* p,size_t n) {
 if(!p||n!=Bytes)return 0;
 uint64_t h=14695981039346656037ull;
 for(size_t i=0;i<n;++i)h=(h^p[i])*1099511628211ull;
 return h;
}
inline bool Retain(bool enabled,bool local,bool engineThread,bool manager,
                   bool supported,bool paused,bool cinematic,int select,int instant,int keep) {
 return enabled&&local&&engineThread&&manager&&supported&&!paused&&!cinematic&&
        (select==0||select==1)&&(instant==0||instant==1)&&keep==0;
}
}
