#pragma once
#include <cstdint>

struct BinkIntroAudioPlan {
    bool enabled=false;
    std::uint32_t count=0, native[8]{}, polish[8]{};
};
inline bool BinkKnownIntroName(const char* filename) {
    if(!filename)return false;
    const char* basename=filename;
    for(const char* p=filename;*p;++p)if(*p=='/' || *p=='\\')basename=p+1;
    const char* expected="sng_intro_12-14-09.bik";
    while(*basename && *expected) {
        char ch=*basename++;
        if(ch>='A' && ch<='Z')ch=char(ch-'A'+'a');
        if(ch!=*expected++)return false;
    }
    return !*basename && !*expected;
}
inline BinkIntroAudioPlan BinkPlanIntroAudio(const char* name, bool polishOn,
    const std::uint32_t* tracks, std::uint32_t count, std::uint32_t flags) {
    BinkIntroAudioPlan plan;
    if(!polishOn || !BinkKnownIntroName(name) || !tracks || count!=4 || flags!=0x01004400u)return plan;
    for(std::uint32_t i=0;i<4;++i)if(tracks[i]!=i)return plan; // only observed native layout
    plan.enabled=true;plan.count=count;
    for(std::uint32_t i=0;i<count;++i)plan.native[i]=plan.polish[i]=tracks[i];
    plan.polish[1]=10; // known intro narration ID, NOT a language ordinal for all movies
    return plan;
}
// Same ordering in production and test. Installed BinkOpen snapshots tracks into
// its handle and resets the global active selection to count1/ID0 on success.
// Preserve that native post-state; restore the pre-Open selection on failure.
template<class SetTracks,class Open>
inline auto BinkOpenWithIntroPlan(const BinkIntroAudioPlan& plan,SetTracks set,Open open) -> decltype(open()) {
    if(plan.enabled)set(plan.count,plan.polish);
    auto handle=open();
    if(plan.enabled) {
        const std::uint32_t zero=0;
        if(handle)set(1,&zero);
        else set(plan.count,plan.native);
    }
    return handle;
}
