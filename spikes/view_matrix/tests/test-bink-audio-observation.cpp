#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../bink_audio_observation.h"
#include <cstdio>
#include <cstring>
int main() {
    int failed=0;
    auto check=[&](bool ok,const char* text){std::printf("%s %s\n",ok?"PASS":"FAIL",text);if(!ok)++failed;};
    DWORD original[3]{0,1,10},copy[8]{};
    check(BinkObserveTracks(3,original,copy) && copy[0]==0 && copy[1]==1 && copy[2]==10,"observed track IDs exact");
    check(original[0]==0 && original[1]==1 && original[2]==10,"native track array untouched");
    check(BinkObserveTracks(0,nullptr,copy),"no-audio track count valid");
    check(!BinkObserveTracks(9,original,copy) && !BinkObserveTracks(1,nullptr,copy),"count/null gates");
    char name[256]{};
    check(BinkObserveName("SNG_intro_12-14-09.bik",name) && !std::strcmp(name,"SNG_intro_12-14-09.bik"),"filename observed exactly");
    const char binary[]{'B','I','K','i',char(0xFF),0};
    check(!BinkObserveName(binary,name) && !name[0],"memory-backed BIK bytes never treated as a pathname");
    auto* inaccessible=VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_NOACCESS);
    check(inaccessible!=nullptr,"private noaccess test page");
    if(inaccessible) {
        check(!BinkObserveTracks(1,static_cast<const DWORD*>(inaccessible),copy),"inaccessible tracks rejected without propagating access violation");
        check(!BinkObserveName(static_cast<const char*>(inaccessible),name) && !name[0],"file-handle/not-name fault contained");
        VirtualFree(inaccessible,0,MEM_RELEASE);
    }
    HANDLE hSelf = CreateFileA(__FILE__, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (hSelf != INVALID_HANDLE_VALUE) {
        check(BinkObserveName(reinterpret_cast<const char*>(hSelf), name, 0x04000000) && std::strstr(name, "test-bink-audio-observation.cpp") != nullptr,
              "file handle resolved to pathname via GetFinalPathNameByHandleA");
        CloseHandle(hSelf);
    }
    return failed?1:0;
}
