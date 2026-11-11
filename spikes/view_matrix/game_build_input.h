#pragma once
#include "game_build_layout.h"

namespace gamebuild {
// Each alternate address was independently matched in the owned German PE.
// Full block matches + native references are documented in the edition audit.
struct InputLayout {
    uintptr_t engine,viewports,mouseX,mouseY,leftButton,padIdentity,mouseSlot;
    uintptr_t poll,pollHandler,cursor[3],padEnum,padCallback,start,stop;
    uint32_t callbackHash,cursorHash[3];
    uintptr_t mouse[4];
    uint32_t mouseHash[4];
    uintptr_t refs[5]; // operands: engine, viewports, MouseX, MouseY, left button
};
inline const InputLayout& InputFor(const Layout& layout){
    static const InputLayout en{
        0x1cf8778,0x1d1fcc4,0x1cf566c,0x1cf5674,0x1cf568c,0x1d1fc7c,0x1d1fc78,
        0x11d31c0,0x170768a,{0x11cfc10,0x11cfc40,0x11cfc70},0x11d2fc0,0x11d2180,0x100d790,0x100aec0,
        0x8735998e,{0x537983a7,0x32189733,0x4a9c5ad3},
        {0x11d7439,0x11d325b,0x11d5f2b,0x11cf3e0},
        {0x72189a6f,0x7d8b63db,0xc02cc0c4,0x807d0696},
        {0xd7c2c1,0x11d2011,0x5e463b,0x11d34d9,0x5ef36a}};
    static const InputLayout de{
        0x1cf7778,0x1d1ecc4,0x1cf466c,0x1cf4674,0x1cf468c,0x1d1ec7c,0x1d1ec78,
        0x11d2e50,0x170728a,{0x11cf8a0,0x11cf8d0,0x11cf900},0x11d2c50,0x11d1e10,0x100d420,0x100ab50,
        0xa3ac90de,{0xaefd9427,0xabd5bfb3,0x9f26e653},
        {0x11d70c9,0x11d2eeb,0x11d5bbb,0x11cf070},
        {0x49a6b01b,0x321a603b,0x20b8ae64,0xa0e0d1a6},
        {0xd7c381,0x11d1ca1,0x5e466b,0x11d3169,0x5ef39a}};
    return layout.calc==Alternate().calc?de:en;
}
inline const InputLayout& Input(){return InputFor(Current());}
inline uint32_t InputHash(const unsigned char* p,unsigned n){
    uint32_t h=2166136261u;for(unsigned i=0;i<n;++i)h=(h^p[i])*16777619u;return h;
}
inline uint64_t InputHash64(const unsigned char* p,unsigned n){
    uint64_t h=14695981039346656037ull;for(unsigned i=0;i<n;++i)h=(h^p[i])*1099511628211ull;return h;
}
template<class Reader> bool MatchesInput(const InputLayout& p,Reader read){
    const unsigned sizes[]{90,1629,32,96};
    for(unsigned i=0;i<4;++i){auto b=read(p.mouse[i],sizes[i]);if(!b||InputHash(b,sizes[i])!=p.mouseHash[i])return false;}
    auto cb=read(p.padCallback,0x313);if(!cb||InputHash(cb,0x313)!=p.callbackHash)return false;
    for(unsigned i=0;i<3;++i){auto b=read(p.cursor[i],48);if(!b||InputHash(b,48)!=p.cursorHash[i])return false;}
    const auto start=read(p.start,248),stop=read(p.stop,194);
    if(!start||!stop||InputHash64(start,248)!=0x894f69cf191c20c7ull||InputHash64(stop,194)!=0x1563d1e2018602deull)return false;
    unsigned char poll[]{0x55,0x8b,0xec,0x6a,0xff,0x68,0,0,0,0};
    uint32_t handler=(uint32_t)p.pollHandler;std::memcpy(poll+6,&handler,4);
    if(!Equal(read(p.poll,sizeof poll),poll,sizeof poll))return false;
    unsigned char enumeration[]{0xa1,0,0,0,0,0x85,0xc0,0x74,0x13,0x8b,0x08,0x8b,0x51,0x10,0x6a,0x01,0x6a,0x00,0x68,0,0,0,0,0x6a,0x04,0x50,0xff,0xd2,0xc3};
    uint32_t identity=(uint32_t)p.padIdentity,callback=(uint32_t)p.padCallback;
    std::memcpy(enumeration+1,&identity,4);std::memcpy(enumeration+19,&callback,4);
    if(!Equal(read(p.padEnum,sizeof enumeration),enumeration,sizeof enumeration))return false;
    const auto mouseOperand=read(p.mouse[3]+1,4);uint32_t mouseAddress=0;
    if(!mouseOperand)return false;std::memcpy(&mouseAddress,mouseOperand,4);
    if(mouseAddress!=p.mouseSlot||!read(p.mouseSlot,4)||!read(p.padIdentity,4))return false;
    const uintptr_t targets[]{p.engine,p.viewports,p.mouseX,p.mouseY,p.leftButton};
    for(unsigned i=0;i<5;++i){
        auto b=read(p.refs[i],4);uint32_t value=0;if(!b)return false;std::memcpy(&value,b,4);
        if(value!=targets[i]||!read(targets[i],8))return false;
    }
    return true;
}
}
