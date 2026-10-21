#include "../bink_intro_audio.h"
#include <cstdio>
#include <cstring>
int main() {
    int failed=0;
    auto check=[&](bool ok,const char* text){std::printf("%s %s\n",ok?"PASS":"FAIL",text);if(!ok)++failed;};
    const std::uint32_t original[]{0,1,2,3};
    const auto planIntro=BinkPlanIntroAudio("..\\RvGame\\Movies\\SNG_intro_12-14-09.bik",true,original,4,0x01004400);
    check(planIntro.enabled && planIntro.count==4,"only known intro/native flags gets Polish plan");
    check(planIntro.polish[0]==0 && planIntro.polish[1]==10 && planIntro.polish[2]==2 && planIntro.polish[3]==3,"replace English narration only; other tracks retained");
    check(original[1]==1 && planIntro.native[1]==1,"caller/native table untouched");
    check(BinkKnownIntroName("C:/Movies/sng_INTRO_12-14-09.BIK"),"basename case/slash handling");
    check(!BinkKnownIntroName("mir12_video.bik"),"intro latch reset restricted to intro movie only");

    // Multilingual cutscene mapping checks:
    const auto planMir12=BinkPlanIntroAudio("..\\RvGame\\Movies\\mir12_video.bik",true,original,4,0x01004400);
    check(planMir12.enabled && planMir12.polish[1]==10,"mir12_video routes to track 10 (Polish)");

    const auto planBad=BinkPlanIntroAudio("badEnding.bik",true,original,4,0x01004400);
    check(planBad.enabled && planBad.polish[1]==10,"badEnding routes to track 10 (Polish)");

    const auto planGood=BinkPlanIntroAudio("goodEnding.bik",true,original,4,0x01004400);
    check(planGood.enabled && planGood.polish[1]==10,"goodEnding routes to track 10 (Polish)");

    const auto planUgly=BinkPlanIntroAudio("uglyEnding.bik",true,original,4,0x01004400);
    check(planUgly.enabled && planUgly.polish[1]==10,"uglyEnding routes to track 10 (Polish)");

    const auto planSave=BinkPlanIntroAudio("saveDemichevRecap.bik",true,original,4,0x01004400);
    check(planSave.enabled && planSave.polish[1]==10,"saveDemichevRecap routes to track 10 (Polish)");

    const auto planBurn=BinkPlanIntroAudio("burningBuildingRecap.bik",true,original,4,0x01004400);
    check(planBurn.enabled && planBurn.polish[1]==10,"burningBuildingRecap routes to track 10 (Polish)");

    const auto planBarisov=BinkPlanIntroAudio("barisovTowerFilmReel.bik",true,original,4,0x01004400);
    check(planBarisov.enabled && planBarisov.polish[1]==9,"barisovTowerFilmReel routes to track 9 (Polish)");

    const auto planMA2=BinkPlanIntroAudio("MA2_5500.bik",true,original,4,0x01004400);
    check(planMA2.enabled && planMA2.polish[1]==8,"MA2_5500 routes to track 8 (Polish)");

    const auto planMA6=BinkPlanIntroAudio("MA6_1000.bik",true,original,4,0x01004400);
    check(planMA6.enabled && planMA6.polish[1]==8,"MA6_1000 routes to track 8 (Polish)");

    const auto planPR99=BinkPlanIntroAudio("PR99_0800.bik",true,original,4,0x01004400);
    check(planPR99.enabled && planPR99.polish[1]==8,"PR99_0800 routes to track 8 (Polish)");

    const auto planVI2=BinkPlanIntroAudio("VI2_3000.bik",true,original,4,0x01004400);
    check(planVI2.enabled && planVI2.polish[1]==8,"VI2_3000 routes to track 8 (Polish)");

    // Unclassified / Ambient movies untouched:
    check(!BinkPlanIntroAudio("SP_Freighter.bik",true,original,4,0x01004400).enabled,"ambient freighter movie untouched");
    check(!BinkPlanIntroAudio("katyaEnding.bik",true,original,4,0x01004400).enabled,"silent post-credits movie untouched");
    check(!BinkPlanIntroAudio("random_movie.bik",true,original,4,0x01004400).enabled,"unclassified movies untouched");

    // Fallbacks:
    check(!BinkPlanIntroAudio("mir12_video.bik",false,original,4,0x01004400).enabled,"Polish OFF native fallback");
    check(!BinkPlanIntroAudio("mir12_video.bik",true,nullptr,4,0x01004400).enabled,"unobserved selection native fallback");
    check(!BinkPlanIntroAudio("mir12_video.bik",true,original,3,0x01004400).enabled,"other layout/count native fallback");
    check(!BinkPlanIntroAudio("mir12_video.bik",true,original,4,0x04004400).enabled,"memory/other flags native fallback");
    std::uint32_t other[]{0,10,2,3};
    check(!BinkPlanIntroAudio("mir12_video.bik",true,other,4,0x01004400).enabled,"already-localized/non-English selection untouched");

    int order=0;std::uint32_t active[8]{};std::uint32_t count=0;
    auto set=[&](std::uint32_t n,const std::uint32_t* ids) {
        ++order;count=n;for(std::uint32_t i=0;i<n;++i)active[i]=ids[i];
        if(order==1)check(n==4 && ids[1]==10,"override precedes Open");
    };
    auto* result=BinkOpenWithIntroPlan(planMir12,set,[&](){
        ++order;check(order==2 && count==4 && active[1]==10,"Open consumes Polish selection");
        return reinterpret_cast<void*>(0x1234);
    });
    check(result==reinterpret_cast<void*>(0x1234) && order==3 && count==1 && active[0]==0,"success keeps native default post-Open selection");
    order=0;
    result=BinkOpenWithIntroPlan(planMir12,set,[&](){++order;return static_cast<void*>(nullptr);});
    check(!result && order==3 && count==4 && active[1]==1,"failed Open rolls selection back");
    int nativeCalls=0;
    result=BinkOpenWithIntroPlan(BinkIntroAudioPlan{},[&](std::uint32_t,const std::uint32_t*){++nativeCalls;},[](){return reinterpret_cast<void*>(0x1234);});
    check(nativeCalls==0 && result==reinterpret_cast<void*>(0x1234),"no plan means exactly one native Open, no setting changes");
    return failed?1:0;
}
