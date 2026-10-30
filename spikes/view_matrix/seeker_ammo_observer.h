#pragma once
#include "native_refill_only.h"
// Observation only: never calls Refill, changes inventory, or grants permits.
namespace seekerammo {
enum class Observation { Waiting, Conserved, Inconclusive, Expired };
struct Transaction {
    nativerefill::Counts before{};
    uint32_t pawn=0,weapon=0;uint64_t started=0,last=0;
    bool active=false;int accepted=0;
    bool Begin(uint32_t p,uint32_t w,nativerefill::Counts c,uint64_t now){
        if(active||!p||!w||!nativerefill::Valid(c)||c.clip==c.capacity||!c.reserve)return false;
        before=c;pawn=p;weapon=w;started=last=now;active=true;accepted=0;return true;
    }
    bool Due(uint64_t now)const{return active&&now>=last&&now-last>=100;}
    Observation Sample(uint32_t p,uint32_t w,nativerefill::Counts c,uint64_t now){
        if(!active||p!=pawn||w!=weapon||now<last||!nativerefill::Valid(c)){
            active=false;return Observation::Inconclusive;
        }
        last=now;
        const int moved=(std::min)(before.capacity-before.clip,before.reserve);
        if(accepted&&c==nativerefill::Counts{before.clip+moved,before.reserve-moved,before.capacity}){
            active=false;return Observation::Conserved;
        }
        if(!(c==before)){active=false;return Observation::Inconclusive;}
        if(now-started>=8000){active=false;return Observation::Expired;}
        return Observation::Waiting;
    }
};
}
