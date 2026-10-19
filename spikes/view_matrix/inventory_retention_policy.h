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
// The latched first-person mesh vertex count of each supported gun. The wheel
// catalog maps the same numbers to sectors; the aliases cover the archetype
// names the package uses (ARPistol/ARMachineGun/ARShotgun/ARSniperRifle) and
// the display names the wheel catalog ships. An unknown name maps to 0 and is
// REPORTED, never guessed.
inline int WeaponMesh(const char* n) {
  if(!n||!n[0])return 0;
  if(!std::strcmp(n,"ARPistol"))return 5799;
  if(!std::strcmp(n,"ARMachineGun"))return 11387;
  if(!std::strcmp(n,"ARShotgun"))return 8297;
  if(!std::strcmp(n,"ARSniperRifle"))return 11389;
  if(!std::strcmp(n,"Centurion"))return 5799;
  if(!std::strcmp(n,"AR9")||!std::strcmp(n,"Valkyrie")||!std::strcmp(n,"ARValkyrie"))return 11387;
  if(!std::strcmp(n,"Shotgun")||!std::strcmp(n,"Spikeshot"))return 8297;
  if(!std::strcmp(n,"Sniper")||!std::strcmp(n,"Kasimov")||!std::strcmp(n,"ARSniper"))return 11389;
  return 0;
}
inline unsigned WeaponBit(int v){return v==5799?1u:v==11387?2u:v==8297?4u:v==11389?8u:0u;}
// RvPlayerPawnSP's audited vtable (0x19D0788 in the owned EXE) resolves
// +0xB14 to the weapon selector and +0xB18 to this HasWeapon target. The
// runtime identifies the vtable by that neighbour instead of by a body hash:
// the installed image is packed, so a byte hash of the method cannot be
// recomputed from the file, while the neighbour pointer can - and it was
// verified offline against the owned EXE (tools/reload/
// verify_pawn_weapon_selector.py).
inline constexpr uintptr_t HasWeaponImpl=0x0102F700ull;
}
