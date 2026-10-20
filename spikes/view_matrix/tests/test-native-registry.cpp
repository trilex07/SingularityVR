#include "../native_registry_policy.h"
#include <cassert>
#include <vector>
int main(){
    std::vector<unsigned char> raw(256,0);
    const char* name="intUTestexecMethod";
    const auto len=std::strlen(name);
    for(std::size_t i=0;i<len;++i)raw[32+i*2]=(unsigned char)name[i];
    const std::uint32_t pointer=0x1020,method=0x401234;
    std::memcpy(raw.data()+192,&pointer,4);std::memcpy(raw.data()+196,&method,4);
    nativeregistry::Region r{raw.data(),raw.size(),0x1000};
    assert(nativeregistry::Find(&r,1,name)==method);
    assert(!nativeregistry::Find(&r,1,"intUTestexecMethodX"));
    raw[32+len*2]='x';assert(!nativeregistry::Find(&r,1,name));raw[32+len*2]=0;
    std::memcpy(raw.data()+208,&pointer,4);assert(!nativeregistry::Find(&r,1,name));
    std::memset(raw.data()+208,0,4);
    std::memcpy(raw.data()+80,raw.data()+32,(len+1)*2);assert(!nativeregistry::Find(&r,1,name));
    std::memset(raw.data()+80,0,(len+1)*2);
    nativeregistry::Region split[]={{raw.data(),128,0x1000},{raw.data()+128,128,0x1080}};
    assert(nativeregistry::Find(split,2,name)==method);
    nativeregistry::Region unaligned{raw.data()+1,raw.size()-1,0x1001};
    assert(nativeregistry::Find(&unaligned,1,name)==method);
    const std::uint32_t zero=0;
    std::memcpy(raw.data()+196,&zero,4);assert(!nativeregistry::Find(&r,1,name));
    std::memcpy(raw.data()+196,&method,4);
    r.size=199;assert(!nativeregistry::Find(&r,1,name));r.size=raw.size();
    char oversized[128];std::memset(oversized,'x',sizeof(oversized));
    assert(!nativeregistry::Find(&r,1,oversized));
    r.address=UINT32_MAX-12;assert(!nativeregistry::Find(&r,1,name));
    assert(!nativeregistry::Find(nullptr,0,name));
}
