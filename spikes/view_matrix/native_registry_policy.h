#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
namespace nativeregistry {
struct Region { const unsigned char* bytes=nullptr;std::size_t size=0;std::uint32_t address=0; };
inline std::uint32_t U32(const unsigned char* p){std::uint32_t v;std::memcpy(&v,p,4);return v;}
// UE3's measured FNativeFunctionLookup is {UTF16 symbol pointer, exec pointer}.
// Require exactly one full NUL-terminated symbol and one aligned lookup row.
// A string hit alone NEVER authorizes a native call; the caller must validate
// the wrapper and every derived helper against their complete fingerprints.
inline std::uint32_t Find(const Region* regions,std::size_t count,const char* symbol){
    if(!regions||!symbol||!count||count>96)return 0;
    std::size_t length=0;while(length<128&&symbol[length])++length;
    if(!length||length==128)return 0;
    std::size_t total=0;
    for(std::size_t i=0;i<count;++i){
        const auto& r=regions[i];
        if(!r.bytes||r.size>32u*1024u*1024u||r.size>UINT32_MAX-r.address)return 0;
        total+=r.size;if(total>64u*1024u*1024u)return 0;
    }
    std::uint32_t name=0;unsigned names=0;
    for(std::size_t i=0;i<count;++i){const auto& r=regions[i];
        for(std::size_t at=(0u-r.address)&1u;at+2*(length+1)<=r.size;at+=2){
            if(r.bytes[at]!=(unsigned char)symbol[0]||r.bytes[at+1])continue;
            std::size_t j=0;
            for(;j<length;++j)if(r.bytes[at+2*j]!=(unsigned char)symbol[j]||r.bytes[at+2*j+1])break;
            if(j==length&&!r.bytes[at+2*j]&&!r.bytes[at+2*j+1]){name=r.address+(std::uint32_t)at;++names;}
        }
    }
    if(names!=1)return 0;
    std::uint32_t method=0;unsigned rows=0;
    for(std::size_t i=0;i<count;++i){const auto& r=regions[i];
        for(std::size_t at=(0u-r.address)&3u;at+8<=r.size;at+=4){
            if(U32(r.bytes+at)==name){method=U32(r.bytes+at+4);++rows;}
        }
    }
    return rows==1?method:0;
}
}
