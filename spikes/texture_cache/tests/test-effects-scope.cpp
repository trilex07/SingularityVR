#include "../remaster_scope.h"
#include <cstdio>
int main(){
    const char* first[] = {"AR_shotgun_d","AR_shotgun_n","inview_glove_d","inview_glove_n"};
    for(const char* name:first)if(!remasterscope::PriorityMaterial(name))return 5;
    if(remasterscope::PriorityMaterial("AR_shotgun_e")||
       remasterscope::PriorityMaterial("handgun_d")||
       remasterscope::PriorityMaterial("AR_shotgun_d_extra")||
       remasterscope::PriorityMaterial(nullptr))return 6;
    if(remasterscope::ParsePriorityMode(-1)!=remasterscope::PriorityMode::Off||
       remasterscope::ParsePriorityMode(0)!=remasterscope::PriorityMode::Off||
       remasterscope::ParsePriorityMode(1)!=remasterscope::PriorityMode::LogOnly||
       remasterscope::ParsePriorityMode(2)!=remasterscope::PriorityMode::Apply||
       remasterscope::ParsePriorityMode(3)!=remasterscope::PriorityMode::Off)return 7;
    using remasterscope::PriorityMode;
    const char* scopeNames[]={"AR_E99Rifle_alt_d","AR_E99Rifle_n","AR_shotgun_d",
        "inview_glove_n","handgun_d","ar_sniper_rifle_d","360_A","AR_E99Rifle_n_extra",nullptr};
    for(const auto* name:scopeNames)for(int p=0;p<3;++p)for(int v=0;v<3;++v)for(int old=0;old<2;++old){
        const auto pm=remasterscope::ParsePriorityMode(p),vm=remasterscope::ParsePriorityMode(v);
        const bool scope=remasterscope::Contains(name);
        const bool actual=remasterscope::UseMaterialRemaster(name,scope,old!=0,pm,vm);
        const bool expected=remasterscope::ValkyrieMaterial(name)?
            (v==1?false:v==2?true:scope&&old):
            remasterscope::UseRemaster(remasterscope::PriorityMaterial(name),scope,old!=0,pm);
        if(actual!=expected)return 10;
        if(v==0 && actual!=remasterscope::UseRemaster(remasterscope::PriorityMaterial(name),scope,old!=0,pm))return 11;
    }
    if(!remasterscope::ValkyrieMaterial("AR_E99Rifle_alt_d")||
       !remasterscope::ValkyrieMaterial("AR_E99Rifle_n")||
       remasterscope::ValkyrieMaterial("AR_E99Rifle_n_extra")||
       remasterscope::ValkyrieMaterial("handgun_d")||remasterscope::ValkyrieMaterial(nullptr))return 12;
    if(remasterscope::UseRemaster(true,true,false,PriorityMode::Off)||
       remasterscope::UseRemaster(true,true,true,PriorityMode::LogOnly)||
       !remasterscope::UseRemaster(true,true,false,PriorityMode::Apply)||
       remasterscope::UseRemaster(false,false,true,PriorityMode::Apply)||
       !remasterscope::UseRemaster(false,true,true,PriorityMode::Off))return 9;
    const char* names[]={"muzzleflashCenter","muzzleFlashDir_01","brightFlash_01","spark_hit01",
        "sparks1_subuv","explosion","fireball_base_intense_02","bsplat3_d","bsplat_decal_d",
        "t_blood_splat02_d","t_decal_metal_d_v4"};
    for(const char* n:names){
        if(remasterscope::Contains(n) || !remasterscope::Contains(n,true))return 1;
    }
    if(remasterscope::Contains(nullptr,true)||remasterscope::Contains("360_A",true)||
       remasterscope::Contains("spark_hit01_extra",true)||remasterscope::Contains("tracer2",true))return 2;
    if(!remasterscope::Contains("AR_shotgun_d")||!remasterscope::Contains("AR_shotgun_d",true))return 3;
    if(!remasterscope::IsAddedBlood("bsplat_decal_d")||!remasterscope::IsAddedBlood("t_blood_splat02_d")||
       remasterscope::IsAddedBlood("bsplat3_d")||remasterscope::IsAddedBlood(nullptr))return 4;
    puts("PASS effects/material scope: 162 mode combinations; default baseline, exact pairs and legacy identities preserved");
    return 0;
}
