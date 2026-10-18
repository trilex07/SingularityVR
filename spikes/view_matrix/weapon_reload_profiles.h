#pragma once
namespace weaponreloadproto {
enum class Kind {None,Valkyrie,Volk,Kasimov};
struct Profile {
    Kind kind;const char* name;const char* objectName;
    long meshWedges;const char* clipParent;
};
inline constexpr Profile profiles[]{
    {Kind::Valkyrie,"AR9 Valkyrie","ARMachineGun",11387,"cog_machinegun"},
    {Kind::Volk,"Volk S4","ARShotgun",8297,"cog_shotgun"},
    {Kind::Kasimov,"Kasimov SNV-E99","ARSniperRifle",11389,"cog_sniper"}
};
inline const Profile* Find(Kind kind){for(const auto& p:profiles)if(p.kind==kind)return &p;return nullptr;}
}
