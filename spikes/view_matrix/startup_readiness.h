#pragma once
#include <cstdint>
#include <algorithm>

namespace startup {
struct Retry {
    unsigned attempts=0;std::uint64_t next=0;
    bool Due(std::uint64_t now,bool eligible,bool missing,unsigned limit=12){
        if(!eligible||!missing||attempts>=limit||now<next)return false;
        ++attempts;next=now+(attempts<4?250:1000);return true;
    }
};
enum Bits:unsigned {Pawn=1,Properties=2,Input=4,Script=8,Aim=16,Centre=32,All=63};
struct Curtain {
    bool begun=false,done=false,timedOut=false;std::uint64_t start=0,stable=0;
    std::uint32_t identity=0;
    bool Update(bool eligible,std::uint32_t pawn,unsigned ready,std::uint64_t now,
                unsigned timeout=5000,unsigned minVisible=0){
        if(!eligible){if(!done){begun=false;stable=0;}return false;}
        if(identity&&pawn&&identity!=pawn){begun=done=timedOut=false;stable=0;}
        if(pawn)identity=pawn;
        if(done)return false;
        if(!begun){begun=true;start=now;stable=0;}
        if(now<start){done=true;timedOut=true;return false;}
        timeout=(std::max)(500u,(std::min)(timeout,10000u));
        minVisible=(std::min)(minVisible,timeout);
        if((ready&All)==All){
            if(!stable)stable=now?now:1;
            if(now>=stable&&now-stable>=100&&now-start>=minVisible){done=true;return false;}
        }else stable=0;
        if(now-start>=timeout){done=true;timedOut=true;return false;}
        return true;
    }
};
} // namespace startup
