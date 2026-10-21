#include "../cinematic_event_bridge.h"
#include <cstdio>
int main(){
    unsigned checks=0;std::uint32_t v[]{1,1,1,1,1,1};
    for(unsigned bits=0;bits<32;++bits){
        const bool got=cineevent::Accept(bits&1,bits&2,bits&4,bits&8,bits&16,v);
        if(got!=(bits==31))return 1;++checks;
    }
    v[0]=0;if(!cineevent::Accept(true,true,true,true,true,v))return 2;++checks;
    for(unsigned i=0;i<6;++i){v[i]=0x1000;
        if(cineevent::Accept(true,true,true,true,true,v))return 3;++checks;v[i]=0;}
    if(cineevent::Accept(true,true,true,true,true,nullptr))return 4;++checks;
    std::printf("PASS %u cinematic-event gates (ON/OFF/refusal, CPU only)\n",checks);
}
