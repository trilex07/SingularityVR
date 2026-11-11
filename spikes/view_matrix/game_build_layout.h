#pragma once
#include <cstdint>
#include <cstring>
#include "native_cinematic_state.h"

namespace gamebuild {
struct Layout {
    const char* name;
    uintptr_t calc,names,objects,line,single;
    uint32_t boolTable;
};
inline const Layout& English(){
    static const Layout p{"english-map",0x0104e390,0x01cd4a1c,0x01cd4a4c,0x00bd79d0,0x00a108b0,0x01cc8730};return p;
}
inline const Layout& Alternate(){
    // Measured GOG German image. Disc image needs the same individual guards;
    // a shared base or one matching prologue is never enough.
    static const Layout p{"alternate-map",0x0104e020,0x01cd3a1c,0x01cd3a4c,0x00bd7a50,0x00a108e0,0x01cc7730};return p;
}
inline Layout& Current(){static Layout p=English();return p;}
// Recognizing the core map does not validate the remaining fixed-address
// consumers (lockers, inventory, scale and RTTI witnesses). Input/menu/mouse/TMD
// contracts now have separate measured profiles, but the DE port is incomplete.
inline bool RuntimeAllowed(const Layout& p){return p.calc==English().calc&&p.names==English().names&&p.objects==English().objects&&p.line==English().line&&p.single==English().single&&p.boolTable==English().boolTable;}
inline bool Equal(const unsigned char* p,const unsigned char* expected,unsigned n){return p&&!std::memcmp(p,expected,n);}
template<class Reader> bool Matches(const Layout& p,Reader read){
    const unsigned char calc[]{0x55,0x8b,0xec,0x83,0xec,0x20,0x8b,0x55,0x14,0xf3,0x0f,0x10,0x45,0x18,0x56,0x33};
    const unsigned char line[]{0x55,0x8b,0xec,0x56,0x8b,0x75,0x08,0x8b,0x4e,0x30,0x85,0xc9,0x74,0x0e,0x8b,0x81,0x40,0x01,0x00,0x00};
    const unsigned char single[]{0x55,0x8b,0xec,0x6a,0xff,0x68,0,0,0,0,0x64,0xa1,0,0,0,0,0x50,0x83,0xec,0x14,0x53,0x56,0xa1,0,0,0,0,0x33,0xc5,0x50,0x8d,0x45,0xf4,0x64,0xa3,0,0,0,0,0xa1,0,0,0,0,0x8b,0x15,0,0};
    const auto singleBytes=read(p.single,sizeof single);
    if(!Equal(read(p.calc,sizeof calc),calc,sizeof calc)||!Equal(read(p.line,sizeof line),line,sizeof line)||
       !singleBytes)return false;
    // Compare the independently matched 48-byte body shape. Absolute operands
    // are edition-specific; instruction bytes and the zero SEH offsets are not.
    for(unsigned i=0;i<sizeof single;++i){
        if((i>=6&&i<10)||(i>=23&&i<27)||(i>=40&&i<44)||i>=46)continue;
        if(singleBytes[i]!=single[i])return false;
    }
    if(!nativecine::SupportedReader(read(0x0048bce0,82),82,p.boolTable))return false;
    // Independently matched native references to each root. These sites agree
    // in both images; their absolute operands differ. Do not derive a blanket delta.
    const auto names=read(0x0042fcb4,5),objects=read(0x00417ed6,6);
    uint32_t namesAddress=0,objectsAddress=0;
    if(!names||names[0]!=0xa1||!objects||objects[0]!=0x8b||objects[1]!=0x0d)return false;
    std::memcpy(&namesAddress,names+1,4);std::memcpy(&objectsAddress,objects+2,4);
    return namesAddress==p.names&&objectsAddress==p.objects&&read(p.names,12)&&read(p.objects,12);
}
template<class Reader> const Layout* Select(Reader read){
    const bool en=Matches(English(),read),alt=Matches(Alternate(),read);
    if(en==alt)return nullptr;return en?&English():&Alternate();
}
}
