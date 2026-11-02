#pragma once
#include "inventory_retention_policy.h"
namespace wheelownership {
// Model observation and a catalogue are not proof of native possession.
inline bool CanOpen(unsigned owned,bool seekerOwned){return owned!=0||seekerOwned;}
inline bool CanSelect(unsigned owned,bool seekerOwned,int mesh){
    return mesh==12444?seekerOwned:(owned&inventoryretention::WeaponBit(mesh))!=0;
}
}
