#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
namespace inventoryretention {
constexpr unsigned ScriptBytes=284,DropAt=200,DropBytes=11;
struct Ref { unsigned at; const char* name; };
inline constexpr Ref Objects[]={
 {13,"foundInv"},{37,"foundInv"},{71,"foundInv"},{103,"foundInv"},{128,"foundInv"},{149,"foundInv"},
 {27,"Archetype"},{117,"Archetype"},{46,"mbInstantSwitchWeapons"},{91,"mbInstantSwitchWeapons"},
 {52,"bInstantSwitch"},{97,"bInstantSwitch"},{84,"bCreateIfNotFound"},{162,"bDropIfNotFound"},
 {66,"Weapon"},{144,"Weapon"},{171,"RvSharedPawn"},
 {176,"Instigator"},{192,"Instigator"},{217,"Instigator"},{244,"Instigator"},{267,"Instigator"},
 {186,"mbDropWeaponOnDeath"},{226,"Weapon"},{253,"Weapon"},{276,"Weapon"},{235,"SetHidden"}};
inline constexpr Ref Names[]={{18,"FindInventoryByArchetype"},{57,"SetCurrentWeapon"},
 {108,"CreateInventoryFromArchetype"},{135,"SetCurrentWeapon"},{201,"ThrowActiveWeapon"}};
inline uint64_t Shape(const unsigned char* code,size_t size) {
 if(!code||size!=ScriptBytes)return 0;
 unsigned char normalized[ScriptBytes];std::memcpy(normalized,code,size);
 for(const auto& r:Objects)std::memset(normalized+r.at,0,4);
 for(const auto& r:Names)std::memset(normalized+r.at,0,8);
 uint64_t h=14695981039346656037ull;
 for(auto b:normalized)h=(h^b)*1099511628211ull;
 return h;
}
inline bool OriginalShape(const unsigned char* code,size_t size) {
 return Shape(code,size)==0xE319C5C3B3C599F8ull;
}
inline bool IconSlot(bool named,bool icon){return named&&icon;}
inline int WeaponMesh(const char* n) {
 if(!n)return 0;
 if(!std::strcmp(n,"ARPistol"))return 5799;
 if(!std::strcmp(n,"ARMachineGun"))return 11387;
 if(!std::strcmp(n,"ARShotgun"))return 8297;
 if(!std::strcmp(n,"ARSniperRifle"))return 11389;
 return 0;
}
inline unsigned WeaponBit(int v){return v==5799?1u:v==11387?2u:v==8297?4u:v==11389?8u:0u;}
}
