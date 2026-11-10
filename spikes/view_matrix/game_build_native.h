#pragma once
#include "game_build_layout.h"

namespace gamebuild {
// Independently measured EN/DE consumers. RuntimeAllowed remains a separate gate.
struct NativeCode { uintptr_t address; unsigned bytes; uint64_t hash; };
struct NativeLayout {
    uintptr_t start,pop,load,equip,loadUpgrades,upgrade,destroy,destroyCall;
    uintptr_t appendName,factory,getGameSystem,hasWeapon,scaleGetter,scaleConsumer;
    uintptr_t lockerVtable,inventoryVtable,gameSystemVtable,gameSystemPtr;
    uintptr_t inventoryGlobal,emptyString,surfaceVtable,textureVtable;
    uintptr_t destroyReturn,scaleReturn,textureConstructor;
    uintptr_t factoryRva,appendRva,gameSystemVtableRva;
    uint64_t factoryHash,scaleGetterHash,scaleConsumerHash;
    unsigned char factoryPrefix[16],loadUpgradesPrefix[16];
    NativeCode code[19];
    uintptr_t refs[7],textureCalls[2];
};
inline const NativeLayout& NativeFor(const Layout& layout){
    static const NativeLayout en{
        0xf11430,0xf17f60,0xf23840,0xf00f50,0xf87c80,0xf87e60,0xa05790,0xf18059,
        0x4affd0,0xfd2320,0xefa070,0x102f700,0xb3e400,0xaec25d,
        0x19a0948,0x19cb480,0x19cdbf8,0x1cf394c,
        0x1cf74e4,0x179e4d0,0x17b4474,0x17b441c,
        0xf1805e,0xaec26c,0x520030,0xbd2320,0xaffd0,0x15cdbf8,
        0xf2b5ebd13e8b220full,0x63ba66a41ada6541ull,0x401d7706085b7ae1ull,
        {0x55,0x8b,0xec,0x6a,0xff,0x68,0x78,0xe9,0x6e,0x1,0x64,0xa1,0x0,0x0,0x0,0x0},
        {0x55,0x8b,0xec,0x6a,0xff,0x68,0x68,0x67,0x6e,0x1,0x64,0xa1,0x0,0x0,0x0,0x0},
        {
            {0xf11430,128,0x7bea8af7932dd60eull},
            {0xf17f60,128,0x8f36d089f196ebd8ull},
            {0xf23840,128,0x2886b390800f1045ull},
            {0xf00f50,128,0x5132dfb739f74679ull},
            {0xf87c80,128,0xa700c6db9dcd5fdull},
            {0xf87e60,128,0x7adfafa714cf409dull},
            {0xa05790,128,0xdab472f7d6de2a4dull},
            {0xf18059,32,0xdcf3d3d1e2f97012ull},
            {0x4affd0,64,0xf7f05a32a361f95full},
            {0xfd2320,128,0x3f4516b0751f0a90ull},
            {0xefa070,32,0x7ff7fc05d10fb9aull},
            {0x102f700,128,0x180c1f390f4df4e8ull},
            {0xb3e400,64,0xe07f4b86341e385full},
            {0xaec25d,26,0x401d7706085b7ae1ull},
            {0x5140e0,498,0xcf890bd6d020569ull},
            {0x514910,302,0x9f8f215f6ecbe68dull},
            {0x520030,128,0xe98e87acc6b28eaull},
            {0x5206a0,32,0x7660672733508b98ull},
            {0x5200c0,100,0x7151778ccc1d6d46ull},
        },
        {0xf9c76c,0x1027914,0x102a29a,0x87bc51,0x5283f7,0x40787d,0x515713},{0x51426e,0x5149da}};
    static const NativeLayout de{
        0xf110c0,0xf17bf0,0xf234d0,0xf00be0,0xf87910,0xf87af0,0xa057c0,0xf17ce9,
        0x4affd0,0xfd1fb0,0xef9d00,0x102f390,0xb3e430,0xaec28d,
        0x199fd20,0x19ca858,0x19ccfd0,0x1cf294c,
        0x1cf64e4,0x179d528,0x17b360c,0x17b35b4,
        0xf17cee,0xaec29c,0x520060,0xbd1fb0,0xaffd0,0x15ccfd0,
        0x2d397e480ed0b86bull,0x63ba66a41ada6541ull,0x401d7706085b7ae1ull,
        {0x55,0x8b,0xec,0x6a,0xff,0x68,0x78,0xe5,0x6e,0x1,0x64,0xa1,0x0,0x0,0x0,0x0},
        {0x55,0x8b,0xec,0x6a,0xff,0x68,0x68,0x63,0x6e,0x1,0x64,0xa1,0x0,0x0,0x0,0x0},
        {
            {0xf110c0,128,0x682bdfb3413aab4ull},
            {0xf17bf0,128,0xdd871c9c2b693774ull},
            {0xf234d0,128,0x63ce77f9aa21c139ull},
            {0xf00be0,128,0xa8bae27edd1ff5b1ull},
            {0xf87910,128,0x9f9969621a48ac49ull},
            {0xf87af0,128,0xed2dee2c6f28f469ull},
            {0xa057c0,128,0xc0325f8f248a3321ull},
            {0xf17ce9,32,0x1232e28576a2f09bull},
            {0x4affd0,64,0xf7f05a32a361f95full},
            {0xfd1fb0,128,0x154c6ab50d0902c3ull},
            {0xef9d00,32,0xa0306630c5c5858aull},
            {0x102f390,128,0x180c1f390f4df4e8ull},
            {0xb3e430,64,0xe07f4b86341e385full},
            {0xaec28d,26,0x401d7706085b7ae1ull},
            {0x514110,498,0x320a5ff90e359f8ull},
            {0x514940,302,0x70d4b4419180bb14ull},
            {0x520060,128,0xf3475c090cce3980ull},
            {0x5206d0,32,0xda6cbbeafbafc308ull},
            {0x5200f0,100,0xde76421e67dcd1daull},
        },
        {0xf9c3fc,0x10275a4,0x1029f2a,0x87bc81,0x528427,0x40787d,0x515743},{0x51429e,0x514a0a}};
    return layout.calc==Alternate().calc?de:en;
}
// Stable storage: references in consumers must survive selection after static initialization.
inline NativeLayout& NativeCurrent(){static NativeLayout p=NativeFor(English());return p;}
inline uint64_t NativeHash(const unsigned char* p,unsigned n){
    uint64_t h=14695981039346656037ull;for(unsigned i=0;i<n;++i)h=(h^p[i])*1099511628211ull;return h;
}
template<class Reader> bool MatchesNative(const NativeLayout& p,Reader read){
    const uintptr_t addresses[]{p.start,p.pop,p.load,p.equip,p.loadUpgrades,p.upgrade,p.destroy,p.destroyCall,
        p.appendName,p.factory,p.getGameSystem,p.hasWeapon,p.scaleGetter,p.scaleConsumer};
    for(unsigned i=0;i<14;++i)if(p.code[i].address!=addresses[i])return false;
    if(p.code[16].address!=p.textureConstructor||p.factoryRva!=p.factory-0x400000||
       p.appendRva!=p.appendName-0x400000||p.gameSystemVtableRva!=p.gameSystemVtable-0x400000)return false;
    for(const auto& c:p.code){auto b=read(c.address,c.bytes);if(!b||NativeHash(b,c.bytes)!=c.hash)return false;}
    auto factory=read(p.factory,16),upgrades=read(p.loadUpgrades,16),getter=read(p.scaleGetter,6),consumer=read(p.scaleConsumer,26);
    if(!factory||!upgrades||!getter||!consumer||std::memcmp(factory,p.factoryPrefix,16)||
       std::memcmp(upgrades,p.loadUpgradesPrefix,16)||NativeHash(getter,6)!=p.scaleGetterHash||
       NativeHash(consumer,26)!=p.scaleConsumerHash)return false;
    // The existing factory witness uses its historical FNV seed, deliberately
    // different from the standard FNV64 seed used by the code contracts above.
    uint64_t factoryHash=1469598103934665603ull;
    for(unsigned i=0;i<16;++i)factoryHash=(factoryHash^factory[i])*1099511628211ull;
    if(factoryHash!=p.factoryHash)return false;
    auto word=[&](uintptr_t a,uint32_t expected){auto b=read(a,4);uint32_t v=0;if(!b)return false;std::memcpy(&v,b,4);return v==expected;};
    const uintptr_t globals[]{p.lockerVtable,p.inventoryVtable,p.gameSystemVtable,p.gameSystemPtr,p.inventoryGlobal,p.emptyString,p.surfaceVtable};
    for(unsigned i=0;i<7;++i)if(!word(p.refs[i],(uint32_t)globals[i])||!read(globals[i],4))return false;
    if(!word(p.lockerVtable+0x11c,(uint32_t)p.start)||!word(p.lockerVtable+0x190,(uint32_t)p.pop)||
       !word(p.lockerVtable+0x1f4,(uint32_t)p.load)||!word(p.lockerVtable+0x1f8,(uint32_t)p.equip)||
       !word(p.gameSystemVtable+0x20c,(uint32_t)p.factory))return false;
    auto call=[&](uintptr_t a,uintptr_t expected){auto b=read(a,5);int32_t displacement=0;if(!b||b[0]!=0xe8)return false;std::memcpy(&displacement,b+1,4);return uintptr_t(int64_t(a)+5+displacement)==expected;};
    if(!call(p.destroyCall,p.destroy)||p.destroyReturn!=p.destroyCall+5||p.scaleReturn!=p.scaleConsumer+15)return false;
    for(auto c:p.textureCalls)if(!call(c,p.textureConstructor))return false;
    if(!word(p.textureVtable,(uint32_t)p.code[17].address)||!call(p.code[17].address+6,p.code[18].address))return false;
    auto destructorStore=read(p.code[18].address+0x29,6);
    if(!destructorStore||destructorStore[0]!=0xc7||destructorStore[1]!=0x06||
       !word(p.code[18].address+0x2b,(uint32_t)p.textureVtable))return false;
    auto store=read(p.textureConstructor+0x67,6);
    return store&&store[0]==0xc7&&store[1]==0x06&&word(p.textureConstructor+0x69,(uint32_t)p.textureVtable)&&read(p.textureVtable,4);
}
}
