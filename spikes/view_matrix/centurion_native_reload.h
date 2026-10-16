#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>

// Audited native AttemptReload seam, NOT ProcessInternal or a UObject ABI guess.
// Evidence and independent offline mapper: docs/centurion-native-reload.md.
namespace centurionnative {
constexpr std::size_t WrapperBytes=58,MethodBytes=0x1d3;
inline bool Bytes(const std::uint8_t* b,std::size_t n,std::size_t at,
                  const std::uint8_t* pattern,std::size_t size){
    return b&&at<=n&&size<=n-at&&std::memcmp(b+at,pattern,size)==0;
}
template<std::size_t N>inline bool At(const std::uint8_t* b,std::size_t n,
                                   std::size_t at,const std::uint8_t (&p)[N]){
    return Bytes(b,n,at,p,N);
}
inline bool DecodeWrapper(const std::uint8_t* b,std::size_t n,std::uint32_t& slot){
    // No params, consumes EndFunctionParms/optional debug token, calls virtual
    // method(this), stores complete EAX into RESULT, ret8. Only the absolute
    // VM debug-handler address and the derived virtual displacement may vary.
    constexpr std::uint8_t prefix[]={0x55,0x8b,0xec,0x8b,0x45,0x08,0xff,0x40,
        0x18,0x56,0x8b,0xf1,0x8b,0x48,0x18,0x80,0x39,0x41,0x75,0x10,
        0x41,0x6a,0x00,0x89,0x48,0x18,0x8b,0x48,0x14,0x50,0xff,0x15};
    constexpr std::uint8_t dispatch[]={0x8b,0x06,0x8b,0x90};
    constexpr std::uint8_t tail[]={0x8b,0xce,0xff,0xd2,0x8b,0x4d,0x0c,0x89,0x01,
        0x5e,0x5d,0xc2,0x08,0x00};
    if(n<WrapperBytes||!At(b,n,0,prefix)||!At(b,n,36,dispatch)||!At(b,n,44,tail))return false;
    std::uint32_t found=0;std::memcpy(&found,b+40,4);
    if(found<0x100||found>0x1000||(found&3))return false;
    slot=found;return true;
}
inline bool AuditedMethod(const std::uint8_t* b,std::size_t n){
    // Reject different versions/overrides; no absolute addresses are accepted.
    // Anchors: stack locals and zero bool, clip<capacity + reserve>0 guards,
    // successful bool store and complete no-stack-argument epilogue.
    constexpr std::uint8_t head[]={0x55,0x8b,0xec,0x83,0xec,0x08,0x8b,0x15};
    constexpr std::uint8_t locals[]={0x53,0x56,0x57,0x33,0xdb,0x53,0x8d,0x45,0xfc,0x50,0x8b,0xf1};
    constexpr std::uint8_t counts[]={0x3b,0x5d,0xfc,0x7d,0x40,0x85,0xc0,0x7e,0x3c,0x8b,0xce,0xe8};
    constexpr std::uint8_t tail[]={0xc7,0x45,0xf8,0x01,0,0,0,0x8b,0x45,0xf8,
        0x5f,0x5e,0x5b,0x8b,0xe5,0x5d,0xc3};
    return n>=MethodBytes&&At(b,n,0,head)&&At(b,n,12,locals)&&At(b,n,0x184,counts)&&At(b,n,0x1c2,tail);
}

enum class Decision {Forward,Observe,Block,Permit};
struct Context {
    bool valid=false;std::uint32_t pawn=0,weapon=0;
    std::uint64_t sampledMs=0;
};
// Pure CPU policy. Runtime supplies a nonblocking lock; this object itself is
// deliberately not shared unsynchronised across the input and game threads.
struct Gate {
    Context current{};
    bool permit=false;std::uint64_t permitMs=0;
    void Clear(){current={};permit=false;permitMs=0;}
    void Publish(const Context& c){
        if(!c.valid||!c.pawn||!c.weapon){Clear();return;}
        if(!current.valid||current.pawn!=c.pawn||current.weapon!=c.weapon||c.sampledMs<current.sampledMs||
           c.sampledMs-current.sampledMs>250)
            permit=false;
        current=c;
    }
    bool Arm(std::uint32_t pawn,std::uint32_t weapon,std::uint64_t now){
        if(!Matches(pawn,weapon,now))return false;
        permit=true;permitMs=now;return true;
    }
    bool Matches(std::uint32_t pawn,std::uint32_t weapon,std::uint64_t now)const{
        return current.valid&&pawn&&weapon&&pawn==current.pawn&&weapon==current.weapon&&
            now>=current.sampledMs&&now-current.sampledMs<=250;
    }
    Decision Evaluate(int mode,std::uint32_t self,std::uint32_t currentPawn,
                      std::uint32_t currentWeapon,std::uint64_t now){
        if(mode<1||mode>2||self!=currentWeapon||!Matches(currentPawn,currentWeapon,now))return Decision::Forward;
        if(mode==1)return Decision::Observe;
        const bool allowed=permit&&now>=permitMs&&now-permitMs<=500;
        permit=false; // consume BEFORE original; nested/duplicate requests cannot reuse it
        return allowed?Decision::Permit:Decision::Block;
    }
};
} // namespace centurionnative
