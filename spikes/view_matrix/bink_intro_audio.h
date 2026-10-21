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

inline std::uint32_t BinkPolishTrackForMovie(const char* filename) {
    if(!filename)return 0;
    const char* basename=filename;
    for(const char* p=filename;*p;++p)if(*p=='/' || *p=='\\')basename=p+1;
    auto match = [](const char* s, const char* exp) {
        while(*s && *exp) {
            char ch=*s++;
            if(ch>='A' && ch<='Z')ch=char(ch-'A'+'a');
            if(ch!=*exp++)return false;
        }
        return !*s && !*exp;
    };

    // 11-track cutscenes: Polish narration is Track 10
    if (match(basename, "sng_intro_12-14-09.bik") ||
        match(basename, "mir12_video.bik") ||
        match(basename, "badending.bik") ||
        match(basename, "goodending.bik") ||
        match(basename, "uglyending.bik") ||
        match(basename, "savedemichevrecap.bik") ||
        match(basename, "burningbuildingrecap.bik")) {
        return 10;
    }

    // 10-track story film reel: Polish narration is Track 9
    if (match(basename, "barisovtowerfilmreel.bik")) {
        return 9;
    }

    // 9-track in-game research projector reels: Polish narration is Track 8
    if (match(basename, "ma2_5500.bik") ||
        match(basename, "ma6_1000.bik") ||
        match(basename, "pr99_0800.bik") ||
        match(basename, "vi2_3000.bik")) {
        return 8;
    }

    return 0; // Not a known multilingual story movie
}

inline BinkIntroAudioPlan BinkPlanIntroAudio(const char* name, bool polishOn,
    const std::uint32_t* tracks, std::uint32_t count, std::uint32_t flags) {
    BinkIntroAudioPlan plan;
    const std::uint32_t polishTrack = BinkPolishTrackForMovie(name);
    if(!polishOn || !polishTrack || !tracks || count!=4 || flags!=0x01004400u)return plan;
    for(std::uint32_t i=0;i<4;++i)if(tracks[i]!=i)return plan; // only observed native layout
    plan.enabled=true;plan.count=count;
    for(std::uint32_t i=0;i<count;++i)plan.native[i]=plan.polish[i]=tracks[i];
    plan.polish[1]=polishTrack; // replace English narration with verified Polish track
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
