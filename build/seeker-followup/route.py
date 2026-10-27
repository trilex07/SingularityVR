from pathlib import Path
p=Path('spikes/view_matrix/d3d9.cpp');s=p.read_text(encoding='utf-8');s=s.replace('{ "InventoryChain", -1 }, { "Inventory", -1 },','{ "InventoryChain", -1 }, { "Inventory", -1 },\n    { "Instigator", -1 }, { "mWeaponName", -1 }, // Seeker ownership cross-checks')
s=s.replace('const bool backGrip=seekerslot::BackGrip(primaryGripRising,','const auto backGate=seekerslot::BackGripGate(primaryGripRising,')
s=s.replace('''    if(backGrip)InterlockedExchange(&g_seekerBackRequest,Rd(g_seekerBackAvailable));''','''    const bool backGrip=backGate==seekerslot::GripGate::Accept;
    if(primaryGripRising&&Rd(g_seekerBackSlot)){
        static unsigned backReceipts=0;
        if(backReceipts++<48){
            const auto zone=bodyequipment::Layout{}.At(bodyequipment::Slot::Heavy,
                {g_centrePos[0],g_centrePos[1],g_centrePos[2]},g_centreYaw);
            Log("Seeker back grip: gate=%s hand=%d tracked=%d availablePawn=%08X handPos=(%.3f %.3f %.3f) zone=(%.3f %.3f %.3f) distance=%.3f radius=%.3f busy[hip=%d support=%d next=%d centurion=%d fire=%d knife=%d wheel=%d]",
                seekerslot::Name(backGate),AimHand(),g_handPositionValid[AimHand()]&&g_handPoseValid[AimHand()],
                (unsigned)Rd(g_seekerBackAvailable),backHand.x,backHand.y,backHand.z,
                zone.centre.x,zone.centre.y,zone.centre.z,hipknife::Length(hipknife::V{backHand.x,backHand.y,backHand.z}-zone.centre),zone.radius,
                Rd(g_weaponHolstered),Rd(g_twoHandActive),g_nextOwnsGrip,g_centurionOwnsGrip,g_nextBlockFire,g_hipKnifeState.held,Rd(g_weaponWheelOpen));
        }
    }
    if(backGrip)InterlockedExchange(&g_seekerBackRequest,Rd(g_seekerBackAvailable));''')
p.write_text(s,encoding='utf-8')
p=Path('spikes/view_matrix/seeker_slot_policy.h');s=p.read_text();start=s.index('inline bool BackGrip(');s=s[:start]+'''enum class GripGate { NoEdge,Context,Tracking,Busy,Unavailable,Outside,Accept };
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
''';p.write_text(s)
