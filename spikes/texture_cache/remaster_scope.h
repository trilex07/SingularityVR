#pragma once
#include <cstring>
namespace remasterscope {
inline bool Contains(const char* name){
    if(!name)return false;
    static const char* names[]={"AR_E99Rifle_alt_d","AR_E99Rifle_n","AR_shotgun_d","AR_shotgun_n",
        "ar_gatling_gun_d","ar_gatling_gun_n","laser_d","laser_n","revert_d2","revert_n2",
        "phasetick_d","phasetick_n","Spetsnaz_MP_helmet_d","Spetsnaz_MP_helmet_n","Spetsnaz_MP_Body_d",
        "Spetsnaz_MP_Body_n","door_rollingT_d","door_rollingT_n","door_blastdoorN_d","door_blastdoorN_n",
        "inview_glove_d","inview_glove_n","elite_beam_n","ar_sniper_rifle_n"};
    for(const auto* entry:names)if(!std::strcmp(name,entry))return true;return false;
}
}
