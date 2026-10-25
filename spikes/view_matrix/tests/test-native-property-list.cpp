#include <windows.h>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
constexpr unsigned OBJ_NAME=0x2c;
alignas(16) static unsigned char cls[0x100],fields[460][0x100];
static bool Readable(const void* p,size_t n){
    auto a=(uintptr_t)p;
    return (a>=(uintptr_t)cls&&a+n<=(uintptr_t)cls+sizeof(cls))||
           (a>=(uintptr_t)fields&&a+n<=(uintptr_t)fields+sizeof(fields));
}
#include "../native_property_list.inl"
int main(){
    static_assert(sizeof(uintptr_t)==4,"actual x86 field layout required");
    // Original Engine.Actor: 458 direct UFields, Location=304, Instigator=417.
    *(uintptr_t*)(cls+0x4c)=(uintptr_t)fields[0];
    for(unsigned i=0;i<458;++i){
        *(int32_t*)(fields[i]+OBJ_NAME)=1000+i;
        *(uintptr_t*)(fields[i]+0x40)=i==457?0:(uintptr_t)fields[i+1];
        *(int32_t*)(fields[i]+0x64)=0xcc;
    }
    assert(PropOffsetUnder((uintptr_t)cls,1000+303,0x4c,0x40,0x64)==0xcc);
    assert(PropOffsetUnder((uintptr_t)cls,1000+416,0x4c,0x40,0x64)==0xcc);
    assert(PropOffsetUnder((uintptr_t)cls,1000+457,0x4c,0x40,0x64)==0xcc);
    assert(PropOffsetUnder((uintptr_t)cls,-1,0x4c,0x40,0x64)==-1);
    assert(PropOffsetUnder((uintptr_t)cls,9999,0x4c,0x40,0x64)==-1);
    *(int32_t*)(fields[416]+0x64)=0x8001;
    assert(PropOffsetUnder((uintptr_t)cls,1416,0x4c,0x40,0x64)==-1);
    *(int32_t*)(fields[416]+0x64)=-1;
    assert(PropOffsetUnder((uintptr_t)cls,1416,0x4c,0x40,0x64)==-1);
    *(uintptr_t*)(fields[457]+0x40)=(uintptr_t)fields[0];
    assert(PropOffsetUnder((uintptr_t)cls,9999,0x4c,0x40,0x64)==-1);
    *(uintptr_t*)(fields[399]+0x40)=1;
    assert(PropOffsetUnder((uintptr_t)cls,1416,0x4c,0x40,0x64)==-1);
    puts("PASS production x86 property walk: Actor child417/458, end/null, invalid offset, unreadable link, bounded cycle");
}
