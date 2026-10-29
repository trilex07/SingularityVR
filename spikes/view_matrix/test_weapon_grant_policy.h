#pragma once
#include <cstdint>
// Explicitly user-authorized test acquisition, never a production unlock rule.
// Only the two requested ordinary weapons; no Seeker, ammo or progression edits.
namespace testweapongrant {
constexpr uint32_t Requested=1u|8u; // Centurion / Kasimov VR ownership bits.
struct Once {
    uint64_t readySince=0,nextAttempt=0;
    unsigned attempts=0;bool done=false;uintptr_t lastPawn=0;
    uint32_t Poll(bool enabled,bool safe,uint32_t owned,uint64_t now,uintptr_t pawn=1){
        if(!enabled||done)return 0;
        if(!safe||!pawn){readySince=0;lastPawn=0;return 0;}
        if(lastPawn!=pawn){readySince=0;lastPawn=pawn;}
        const uint32_t missing=Requested&~owned;
        if(!missing){done=true;return 0;}
        if(!readySince){readySince=now;return 0;}
        if(now<readySince||now-readySince<2500||now<nextAttempt||attempts>=3)return 0;
        ++attempts;nextAttempt=now+2000;return missing;
    }
};
}
