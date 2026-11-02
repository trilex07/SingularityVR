#pragma once
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include "script_node_candidate.h"

// Built once before enabling Bink hooks. Exact local 512-byte prefixes include
// the BIKi header, complete audio ID table and frame index. No movie bytes or
// generated proprietary fingerprints are distributed. Unknown/ambiguous/faulting
// sources keep native playback. No file reads or allocations inside BinkOpen.
struct BinkMemoryMovies {
    static constexpr unsigned PrefixBytes=512;
    struct Entry {const char* name;unsigned char bytes[PrefixBytes]{};bool valid=false;};
    Entry entries[12]{
        {"SNG_intro_12-14-09.bik"},{"mir12_video.bik"},{"badEnding.bik"},
        {"goodEnding.bik"},{"uglyEnding.bik"},{"saveDemichevRecap.bik"},
        {"burningBuildingRecap.bik"},{"barisovTowerFilmReel.bik"},
        {"MA2_5500.bik"},{"MA6_1000.bik"},{"PR99_0800.bik"},{"VI2_3000.bik"}};
    static std::uint32_t U32(const unsigned char* p){std::uint32_t v;std::memcpy(&v,p,4);return v;}
    static bool ValidHeader(const unsigned char* p,ULONGLONG fileBytes){
        if(std::memcmp(p,"BIKi",4) || fileBytes<PrefixBytes || ULONGLONG(U32(p+4))+8!=fileBytes)return false;
        const unsigned count=U32(p+40);
        if(count<9 || count>11 || 44+12*count>PrefixBytes)return false;
        for(unsigned i=0;i<count;++i)if(U32(p+44+8*count+4*i)==10)return true;
        return false;
    }
    unsigned Load(const char* directory){
        unsigned ready=0;
        for(auto& e:entries){
            e.valid=false;char path[MAX_PATH]{};
            if(!directory || std::strlen(directory)+std::strlen(e.name)+2>=MAX_PATH)continue;
            sprintf_s(path,"%s\\%s",directory,e.name);
            HANDLE f=CreateFileA(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
            if(f==INVALID_HANDLE_VALUE)continue;
            LARGE_INTEGER size{};DWORD read=0;
            const bool ok=GetFileSizeEx(f,&size) && ReadFile(f,e.bytes,PrefixBytes,&read,nullptr) &&
                read==PrefixBytes && ValidHeader(e.bytes,size.QuadPart);
            CloseHandle(f);e.valid=ok;if(ok)++ready;
        }
        return ready;
    }
    const char* Identify(const char* source,DWORD flags) const {
        if(!source || flags!=0x04004400u)return nullptr;
        unsigned char snapshot[PrefixBytes];
        __try {std::memcpy(snapshot,source,PrefixBytes);}
        __except(MemoryFaultFilter(GetExceptionCode())) {return nullptr;}
        const char* found=nullptr;
        for(const auto& e:entries)if(e.valid && !std::memcmp(snapshot,e.bytes,PrefixBytes)){
            if(found)return nullptr; // ambiguity never changes native audio
            found=e.name;
        }
        return found;
    }
};
