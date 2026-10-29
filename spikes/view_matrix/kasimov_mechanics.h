#pragma once
#include "volk_mechanics.h"
namespace kasimovmechanics {
// The existing Sample() resolves wire_1 -> wire_2 -> wire_3 before producing
// inverse-bind deltas. Apply their original return only inside the existing
// seating window; physical clip, native Trigger and controller root stay owned.
inline bool Seat(const weaponreloadanim::Data& d,const weaponreload::Animation& timing,
        weaponreload::Phase phase,uint64_t inserted,uint64_t now,
        const float* parent,unsigned regs,float* changed){
    if(d.kind!=weaponreload::Kind::Kasimov||!parent||!changed||regs!=18)return false;
    if(!inserted||now<inserted||now-inserted>=weaponreload::Animation::RefillSeatMs||
       (weaponreload::Active(phase)&&phase!=weaponreload::Phase::WaitingNative))return true;
    armbind::Mat root;if(!armbind::FromGpu(reinterpret_cast<const float(*)[4]>(parent),root))return false;
    float staged[72];memcpy(staged,changed,sizeof(staged));
    const float frame=timing.RefillFrame(d,phase,inserted,inserted,now);
    for(unsigned slot:{1u,2u,3u}){
        armbind::Mat delta,skin;
        if(!volkmechanics::Relative(d,slot,frame,float(d.frames-1),delta))return false;
        skeletal::Multiply(skin.m,delta.m,root.m);
        armbind::ToGpu(skin,reinterpret_cast<float(*)[4]>(staged+slot*12));
    }
    memcpy(changed,staged,sizeof(staged));return true;
}
}
