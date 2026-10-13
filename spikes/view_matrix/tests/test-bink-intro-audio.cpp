#include "../bink_intro_audio.h"
#include <cstdio>
#include <cstring>
int main() {
    int failed=0;
    auto check=[&](bool ok,const char* text){std::printf("%s %s\n",ok?"PASS":"FAIL",text);if(!ok)++failed;};
    const std::uint32_t original[]{0,1,2,3};
    const auto plan=BinkPlanIntroAudio("..\\RvGame\\Movies\\SNG_intro_12-14-09.bik",true,original,4,0x01004400);
    check(plan.enabled && plan.count==4,"only known intro/native flags gets Polish plan");
    check(plan.polish[0]==0 && plan.polish[1]==10 && plan.polish[2]==2 && plan.polish[3]==3,"replace English narration only; other tracks retained");
    check(original[1]==1 && plan.native[1]==1,"caller/native table untouched");
    check(BinkKnownIntroName("C:/Movies/sng_INTRO_12-14-09.BIK"),"basename case/slash handling");
    check(!BinkPlanIntroAudio("badEnding.bik",true,original,4,0x01004400).enabled,"unclassified movies untouched");
    check(!BinkPlanIntroAudio("SNG_intro_12-14-09.bik",false,original,4,0x01004400).enabled,"Polish OFF native fallback");
    check(!BinkPlanIntroAudio("SNG_intro_12-14-09.bik",true,nullptr,4,0x01004400).enabled,"unobserved selection native fallback");
    check(!BinkPlanIntroAudio("SNG_intro_12-14-09.bik",true,original,3,0x01004400).enabled,"other layout/count native fallback");
    check(!BinkPlanIntroAudio("SNG_intro_12-14-09.bik",true,original,4,0x04004400).enabled,"memory/other flags native fallback");
    std::uint32_t other[]{0,10,2,3};
    check(!BinkPlanIntroAudio("SNG_intro_12-14-09.bik",true,other,4,0x01004400).enabled,"already-localized/non-English selection untouched");
    int order=0;std::uint32_t active[8]{};std::uint32_t count=0;
    auto set=[&](std::uint32_t n,const std::uint32_t* ids) {
        ++order;count=n;for(std::uint32_t i=0;i<n;++i)active[i]=ids[i];
        if(order==1)check(n==4 && ids[1]==10,"override precedes Open");
    };
    auto* result=BinkOpenWithIntroPlan(plan,set,[&](){
        ++order;check(order==2 && count==4 && active[1]==10,"Open consumes Polish selection");
        return reinterpret_cast<void*>(0x1234);
    });
    check(result==reinterpret_cast<void*>(0x1234) && order==3 && count==1 && active[0]==0,"success keeps native default post-Open selection");
    order=0;
    result=BinkOpenWithIntroPlan(plan,set,[&](){++order;return static_cast<void*>(nullptr);});
    check(!result && order==3 && count==4 && active[1]==1,"failed Open rolls selection back");
    int nativeCalls=0;
    result=BinkOpenWithIntroPlan(BinkIntroAudioPlan{},[&](std::uint32_t,const std::uint32_t*){++nativeCalls;},[](){return reinterpret_cast<void*>(0x1234);});
    check(nativeCalls==0 && result==reinterpret_cast<void*>(0x1234),"no plan means exactly one native Open, no setting changes");
    return failed?1:0;
}
