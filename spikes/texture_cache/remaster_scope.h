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
// Independently reviewed weapon pairs never widen the accepted Volk/glove gate.
inline bool ValkyrieMaterial(const char* name){
    return name && (!std::strcmp(name,"AR_E99Rifle_alt_d") ||
        !std::strcmp(name,"AR_E99Rifle_n"));
}
// Legacy runtime capture aliases are intentionally retained: their full mip0
// hashes equal original wp_SP_ARPistol_SF AR_Pistol_d/n byte for byte.
inline bool CenturionMaterial(const char* name){
    return name && (!std::strcmp(name,"handgun_d") || !std::strcmp(name,"handgun_n"));
}
inline bool KasimovMaterial(const char* name){
    return name && (!std::strcmp(name,"ar_sniper_rifle_d") || !std::strcmp(name,"ar_sniper_rifle_n"));
}
inline bool ReviewTargetEnabled(const char* name,PriorityMode kasimov){
    // Newly registered diffuse must not introduce a baseline substitution.
    return !name || std::strcmp(name,"ar_sniper_rifle_d") || kasimov!=PriorityMode::Off;
}
inline PriorityMode ParsePriorityMode(int value){
    return value==1?PriorityMode::LogOnly:value==2?PriorityMode::Apply:PriorityMode::Off;
}
inline bool UseRemaster(bool priority,bool inRemasterScope,bool oldApply,PriorityMode mode){
    if(priority && mode==PriorityMode::LogOnly)return false;
    return (priority && mode==PriorityMode::Apply) || (inRemasterScope && oldApply);
}
inline PriorityMode MaterialMode(const char* name,PriorityMode priority,PriorityMode valkyrie,
    PriorityMode centurion=PriorityMode::Off,PriorityMode kasimov=PriorityMode::Off){
    return PriorityMaterial(name)?priority:ValkyrieMaterial(name)?valkyrie:
        CenturionMaterial(name)?centurion:KasimovMaterial(name)?kasimov:PriorityMode::Off;
}
inline bool UseMaterialRemaster(const char* name,bool inRemasterScope,bool oldApply,
                               PriorityMode priority,PriorityMode valkyrie,
                               PriorityMode centurion=PriorityMode::Off,PriorityMode kasimov=PriorityMode::Off){
    return UseRemaster(PriorityMaterial(name)||ValkyrieMaterial(name)||CenturionMaterial(name)||KasimovMaterial(name),inRemasterScope,
                       oldApply,MaterialMode(name,priority,valkyrie,centurion,kasimov));
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
