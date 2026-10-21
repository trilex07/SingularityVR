#pragma once
#include <cstdint>
#include <cstring>

namespace nativecine {
// Singularity execBoolVariable (VM opcode 0x2d): +0x84 is read as the
// property's mask, then TESTed against the resolved instance word. This is
// NOT the layout from another UE3 game. Refuse unsupported executables.
constexpr unsigned BoolExecRva=0x8bce0, MaskMember=0x84;
inline bool SupportedReader(const unsigned char* p, unsigned size) {
    const unsigned char prefix[]{0x55,0x8b,0xec,0x8b,0x45,0x08,0x56,0x8b,0xf1,
        0x8b,0x48,0x18,0x8a,0x11,0x57,0x41,0x89,0x48,0x18,0x8b,0x39,
        0x0f,0xb6,0xd2,0x6a,0x00,0x50,0x8b,0x04,0x95,0x30,0x87,0xcc,0x01};
    const unsigned char maskRead[]{0x8b,0x97,0x84,0x00,0x00,0x00,0x85,0x11,
        0x74,0x0d,0xb9,0x01,0x00,0x00,0x00};
    return p && size>=82 && !std::memcmp(p,prefix,sizeof(prefix)) &&
        !std::memcmp(p+67,maskRead,sizeof(maskRead));
}
inline bool ValidField(int offset, int elements, int size, uint32_t mask) {
    return offset>=0x40 && offset<=0x4000 && !(offset%4) && elements==1 && size==4 &&
        mask && !(mask&(mask-1));
}
inline bool Active(uint32_t word,uint32_t mask) { return (word&mask)!=0; }
// Singularity Matinee/Kathryn segments can run with bCinematicMode still clear while
// SetCinematicMode already latched the mod ON. Native polling may only ASSERT ON.
inline bool PublishesOn(int nativeActive) { return nativeActive!=0; }
}
