#include "../cinematic_state_observation.h"
#include <cstdio>
#include <cstring>
int main() {
    unsigned char controller[0x40]{},frame[0x30]{},node[0x40]{};
    auto store=[](unsigned char* target,int at,std::uintptr_t value){std::memcpy(target+at,&value,sizeof(value));};
    store(controller,0x34,0x10000);store(controller,0x18,(std::uintptr_t)frame);
    store(frame,0x28,(std::uintptr_t)node);store(node,0x34,0x20000);
    const int name=553;std::memcpy(node+0x2c,&name,4);
    const auto good=ReadCinematicStateObservation((std::uintptr_t)controller);
    if(!good.valid || good.stateName!=name || good.stateNode!=(std::uintptr_t)node)return 1;
    if(ReadCinematicStateObservation(0).valid || ReadCinematicStateObservation(1).valid)return 2;
    void* denied=VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_NOACCESS);
    if(!denied || ReadCinematicStateObservation((std::uintptr_t)denied).valid)return 3;
    store(controller,0x18,(std::uintptr_t)denied);
    if(ReadCinematicStateObservation((std::uintptr_t)controller).valid)return 4;
    store(controller,0x18,(std::uintptr_t)frame);store(frame,0x28,(std::uintptr_t)denied);
    if(ReadCinematicStateObservation((std::uintptr_t)controller).valid)return 5;
    VirtualFree(denied,0,MEM_RELEASE);
    if(CinematicObservationFaultFilter(0xe0000042)!=EXCEPTION_CONTINUE_SEARCH)return 6;
    std::puts("PASS: native state chain / null / misalignment / inaccessible controller, frame and node / nonmemory exception");
    return 0;
}
