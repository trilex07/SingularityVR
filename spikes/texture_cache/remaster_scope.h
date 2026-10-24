#pragma once
#include <cstring>
namespace remasterscope {
// First physical A/B is limited to these four true-mip0 first-person maps.
// Keep the accepted base override path unchanged when this experiment is OFF.
inline bool PriorityMaterial(const char* name){
    return name && (!std::strcmp(name,"AR_shotgun_d") ||
        !std::strcmp(name,"AR_shotgun_n") ||
        !std::strcmp(name,"inview_glove_d") ||
        !std::strcmp(name,"inview_glove_n"));
}
enum class PriorityMode { Off=0, LogOnly=1, Apply=2 };
inline PriorityMode ParsePriorityMode(int value){
    return value==1?PriorityMode::LogOnly:value==2?PriorityMode::Apply:PriorityMode::Off;
}
inline bool UseRemaster(bool priority,bool inRemasterScope,bool oldApply,PriorityMode mode){
    if(priority && mode==PriorityMode::LogOnly)return false;
    return (priority && mode==PriorityMode::Apply) || (inRemasterScope && oldApply);
}
inline bool IsAddedBlood(const char* name){
    return name && (!std::strcmp(name,"bsplat_decal_d") || !std::strcmp(name,"t_blood_splat02_d"));
}
inline bool EffectsContains(const char* name){
    if(!name)return false;
    static const char* names[]={"muzzleflashCenter","muzzleFlashDir_01","brightFlash_01","spark_hit01",
        "sparks1_subuv","explosion","fireball_base_intense_02","bsplat3_d","bsplat_decal_d",
        "t_blood_splat02_d","t_decal_metal_d_v4"};
    for(const auto* entry:names)if(!std::strcmp(name,entry))return true;return false;
}
inline bool Contains(const char* name,bool effects=false){
    if(!name)return false;
    if(effects && EffectsContains(name))return true;
    static const char* names[]={"AR_E99Rifle_alt_d","AR_E99Rifle_n","AR_shotgun_d","AR_shotgun_n",
        "ar_gatling_gun_d","ar_gatling_gun_n","laser_d","laser_n","revert_d2","revert_n2",
        "phasetick_d","phasetick_n","Spetsnaz_MP_helmet_d","Spetsnaz_MP_helmet_n","Spetsnaz_MP_Body_d",
        "Spetsnaz_MP_Body_n","door_rollingT_d","door_rollingT_n","door_blastdoorN_d","door_blastdoorN_n",
        "inview_glove_d","inview_glove_n","elite_beam_n","ar_sniper_rifle_n"};
    for(const auto* entry:names)if(!std::strcmp(name,entry))return true;return false;
}
}
