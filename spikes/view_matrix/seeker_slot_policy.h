#pragma once
#include <cstdint>
#include "body_equipment_policy.h"
// A slot is a view of the engine's possession chain, never a second inventory.
namespace seekerslot {
struct Item { uint32_t object=0; int name=0,number=0; bool seeker=false; };
struct Snapshot { uint32_t pawn=0,current=0; Item items[128]{}; unsigned count=0; bool valid=false; };
struct State {
    uint32_t pawn=0,seeker=0,ordinary=0;
    void Reconcile(const Snapshot& s) {
        if(!s.valid||!s.pawn||s.count>128){*this={};return;}
        if(pawn!=s.pawn){*this={};pawn=s.pawn;}
        uint32_t found=0;bool oldOrdinary=false;
        for(unsigned i=0;i<s.count;++i){
            const auto& it=s.items[i];
            if(it.seeker){if(found){*this={};return;}found=it.object;}
            else {if(it.object==ordinary)oldOrdinary=true;if(it.object==s.current)ordinary=it.object;}
        }
        seeker=found;
        if(ordinary!=s.current&&!oldOrdinary)ordinary=0;
    }
    uint32_t ToggleTarget(const Snapshot& s) const {
        if(!s.valid||pawn!=s.pawn||!seeker)return 0;
        if(s.current!=seeker)return seeker;
        for(unsigned i=0;i<s.count;++i)if(!s.items[i].seeker&&s.items[i].object==ordinary)return ordinary;
        for(unsigned i=0;i<s.count;++i)if(!s.items[i].seeker)return s.items[i].object;
        return 0; // no invented empty weapon or item grant
    }
};
inline bool RetainQuery(bool enabled,bool gameplay,bool sameThread,bool ownedSeeker,
                        uintptr_t caller,uintptr_t switchMethod){
    // Both IsHeavyWeapon calls in the fully fingerprinted native SwitchWeapon.
    // They alone select the clear-slot + OnRvThrowActiveWeapon branch.
    return enabled&&gameplay&&sameThread&&ownedSeeker&&switchMethod&&
        (caller==switchMethod+0xd2||caller==switchMethod+0xe9);
}
enum class GripGate { NoEdge,Context,Tracking,Busy,Unavailable,Outside,Accept };
inline const char* Name(GripGate g){
    switch(g){case GripGate::NoEdge:return "no-edge";case GripGate::Context:return "context";
    case GripGate::Tracking:return "tracking";case GripGate::Busy:return "busy";
    case GripGate::Unavailable:return "unavailable";case GripGate::Outside:return "outside";
    default:return "accept";}
}
inline GripGate BackGripGate(bool edge,bool context,bool tracked,bool busy,bool available,
                     hipknife::V hand,hipknife::V head,float yaw){
    if(!edge)return GripGate::NoEdge;if(!context)return GripGate::Context;
    if(!tracked)return GripGate::Tracking;if(busy)return GripGate::Busy;
    if(!available)return GripGate::Unavailable;
    const auto z=bodyequipment::Layout{}.At(bodyequipment::Slot::Heavy,head,yaw);
    return bodyequipment::Pick(hand,&z,1)==bodyequipment::Slot::Heavy?GripGate::Accept:GripGate::Outside;
}
inline bool BackGrip(bool edge,bool context,bool tracked,bool busy,bool available,
                     hipknife::V hand,hipknife::V head,float yaw){
    return BackGripGate(edge,context,tracked,busy,available,hand,head,yaw)==GripGate::Accept;
}
}
