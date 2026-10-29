#pragma once
#include "volk_mechanics.h"
namespace valkyriemechanics {
// Keep the accepted FSM's clip seating and controller-owned root. Only the
// source-authored Slide/fan return segment is applied in its existing 180ms
// refill seat window, normalized to its COMPLETE original endpoint transform.
// Release and Trigger, native first-shot animation outside this window, and
// all manually carried magazine poses remain byte exact.
inline bool Seat(const weaponreloadanim::Data& d,const weaponreload::Animation& timing,
        weaponreload::Phase phase,uint64_t inserted,uint64_t now,
        const float* parent,unsigned regs,float* changed){
    if(d.kind!=weaponreload::Kind::Valkyrie||!parent||!changed||regs!=18)return false;
    if(!inserted||now<inserted||now-inserted>=weaponreload::Animation::RefillSeatMs||
       (weaponreload::Active(phase)&&phase!=weaponreload::Phase::WaitingNative))return true;
    armbind::Mat root;if(!armbind::FromGpu(reinterpret_cast<const float(*)[4]>(parent),root))return false;
    float staged[72];memcpy(staged,changed,sizeof(staged));
    const float frame=timing.RefillFrame(d,phase,inserted,inserted,now);
    for(unsigned slot:{4u,5u}){
        armbind::Mat delta,skin;
        if(!volkmechanics::Relative(d,slot,frame,float(d.frames-1),delta))return false;
        skeletal::Multiply(skin.m,delta.m,root.m);
        armbind::ToGpu(skin,reinterpret_cast<float(*)[4]>(staged+slot*12));
    }
    memcpy(changed,staged,sizeof(staged));return true;
}
}
