#pragma once
#include "centurion_native_reload.h"
#include <algorithm>

// Original no-param native thunks, NOT ProcessInternal. No game bodies/assets
// distributed: fingerprints cover all method bytes except the one cast rel32.
namespace centurionrefill {
struct Audit {
    const char* stage="not-run";
    unsigned matched=0,visited=0,index=4;
    std::uint32_t klass=0,field=0,wrapper=0,method=0,slot=0;
    std::uint64_t fingerprint=0;
}; // bounded resolver evidence, NEVER a permission to call a failed binding
struct Spec {const char* name;std::size_t bytes,rel;std::uint64_t fingerprint;};
constexpr Spec Specs[]={
    {"GetAmmoInClip",53,14,0x814290bcd87ed31dULL},
    {"GetAmmoInBin",53,14,0x9d0fbaa54f040869ULL},
    {"GetClipSize",77,23,0x837eb5e669b80c72ULL},
    {"RefillClip",164,12,0x5626d93f8679565fULL}
};
inline std::uint64_t Fingerprint(const std::uint8_t* b,std::size_t n,std::size_t rel){
    if(!b||rel<1||rel>n||n-rel<4||b[rel-1]!=0xe8)return 0;
    std::uint64_t h=14695981039346656037ULL;
    for(std::size_t i=0;i<n;++i)h=(h^((i>=rel&&i<rel+4)?0:b[i]))*1099511628211ULL;
    return h;
}
inline bool Audited(const std::uint8_t* b,std::size_t n,const Spec& s){
    return n>=s.bytes&&Fingerprint(b,s.bytes,s.rel)==s.fingerprint;
}
inline bool DecodeVoidWrapper(const std::uint8_t* b,std::size_t n,std::uint32_t& slot){
    constexpr std::uint8_t prefix[]={0x55,0x8b,0xec,0x8b,0x45,0x08,0xff,0x40,
        0x18,0x56,0x8b,0xf1,0x8b,0x48,0x18,0x80,0x39,0x41,0x75,0x10,
        0x41,0x6a,0,0x89,0x48,0x18,0x8b,0x48,0x14,0x50,0xff,0x15};
    constexpr std::uint8_t dispatch[]={0x8b,0x06,0x8b,0x90};
    constexpr std::uint8_t tail[]={0x8b,0xce,0xff,0xd2,0x5e,0x5d,0xc2,8,0};
    if(n<53||!centurionnative::At(b,n,0,prefix)||!centurionnative::At(b,n,36,dispatch)||
       !centurionnative::At(b,n,44,tail))return false;
    std::uint32_t s=0;std::memcpy(&s,b+40,4);
    if(s<0x100||s>0x1000||(s&3))return false;
    slot=s;return true;
}
struct Counts {int clip=-1,reserve=-1,capacity=-1;};
inline bool operator==(Counts a,Counts b){return a.clip==b.clip&&a.reserve==b.reserve&&a.capacity==b.capacity;}
inline bool Valid(Counts a){return a.capacity==6&&a.clip>=0&&a.clip<=a.capacity&&a.reserve>=0&&a.reserve<=100000;}
enum class Result {Rejected,Changed,Conserved,Fault};
// Caller consumes identity/lease permit BEFORE this operation, on the natural
// engine thread. Read/Refill are injected for L1; runtime uses audited thiscall.
// No native full-reload fallback, even if postcondition fails after the call.
template<class Native> Result Commit(Counts expected,Native& native){
    if(!Valid(expected)||expected.clip==expected.capacity||expected.reserve==0)return Result::Rejected;
    const Counts before=native.Read();
    if(!Valid(before)||!(before==expected))return Result::Changed;
    const int moved=(std::min)(before.capacity-before.clip,before.reserve);
    native.Refill();
    const Counts after=native.Read();
    return after==Counts{before.clip+moved,before.reserve-moved,before.capacity}?
        Result::Conserved:Result::Fault;
}
} // namespace centurionrefill
