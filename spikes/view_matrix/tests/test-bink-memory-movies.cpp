#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../bink_memory_movies.h"
#include "../bink_intro_audio.h"
#include <vector>
#include <fstream>
#include <iterator>
static int failures=0;
static void Check(bool ok,const char* message){printf("%s %s\n",ok?"PASS":"FAIL",message);if(!ok)++failures;}
int main(int argc,char** argv){
    BinkMemoryMovies catalog;
    auto& entry=catalog.entries[8]; // MA2: actual file has IDs 0,1,4..10
    entry.bytes[0]='B';entry.bytes[1]='I';entry.bytes[2]='K';entry.bytes[3]='i';
    auto put=[&](unsigned offset,unsigned value){std::memcpy(entry.bytes+offset,&value,4);};
    put(4,4096-8);put(40,9);
    const unsigned ids[]{0,1,4,5,6,7,8,9,10};
    for(unsigned i=0;i<9;++i)put(44+8*9+4*i,ids[i]);
    Check(BinkMemoryMovies::ValidHeader(entry.bytes,4096),"non-contiguous IDs validate actual Polish ID 10");
    Check(!BinkMemoryMovies::ValidHeader(entry.bytes,4097),"size mismatch fails closed");
    put(44+8*9+4*8,8);Check(!BinkMemoryMovies::ValidHeader(entry.bytes,4096),"ordinal 8 is not Polish ID 10");put(44+8*9+4*8,10);
    entry.valid=true;
    const char* name=catalog.Identify(reinterpret_cast<char*>(entry.bytes),0x04004400);
    Check(name && !strcmp(name,"MA2_5500.bik"),"memory identity uses exact complete local prefix");
    unsigned char changed[512];memcpy(changed,entry.bytes,512);changed[511]^=1;
    Check(!catalog.Identify(reinterpret_cast<char*>(changed),0x04004400),"same header with different frame index rejected");
    Check(!catalog.Identify(reinterpret_cast<char*>(entry.bytes),0x01004400),"filename mode never interpreted as memory");
    catalog.entries[9]=entry;
    Check(!catalog.Identify(reinterpret_cast<char*>(entry.bytes),0x04004400),"ambiguous catalog never changes audio");catalog.entries[9].valid=false;
    void* inaccessible=VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_NOACCESS);
    Check(inaccessible!=nullptr,"fault fixture allocated");
    if(inaccessible){Check(!catalog.Identify(static_cast<char*>(inaccessible),0x04004400),"inaccessible source preserves native forwarding");VirtualFree(inaccessible,0,MEM_RELEASE);}
    Check(catalog.Load("Z:\\missing-bink-catalog")==0,"missing local movies leave native playback");
    if(argc==3){
        catalog=BinkMemoryMovies{}; // discard intentional duplicate synthetic names
        Check(catalog.Load(argv[1])==12,"all twelve installed local story identities cached");
        for(const auto& e:catalog.entries){
            if(!e.valid){Check(false,e.name);continue;}
            const auto count=BinkMemoryMovies::U32(e.bytes+40);
            const char* found=catalog.Identify(reinterpret_cast<const char*>(e.bytes),0x04004400);
            const std::uint32_t tracks[]{0,1,2,3};
            const auto plan=BinkPlanIntroAudio(found,true,tracks,4,0x04004400);
            Check(found && plan.enabled && plan.polish[1]==10 &&
                BinkMemoryMovies::U32(e.bytes+44+8*count+4*(count-1))==10,e.name);
        }
        HMODULE dll=LoadLibraryA(argv[2]);Check(dll!=nullptr,"installed codec loads offline without game launch");
        if(dll){
            const auto open=reinterpret_cast<void* (WINAPI*)(const char*,DWORD)>(GetProcAddress(dll,"_BinkOpen@8"));
            const auto close=reinterpret_cast<void (WINAPI*)(void*)>(GetProcAddress(dll,"_BinkClose@4"));
            const auto id=reinterpret_cast<DWORD (WINAPI*)(void*,DWORD)>(GetProcAddress(dll,"_BinkGetTrackID@8"));
            const auto set=reinterpret_cast<void (WINAPI*)(DWORD,const DWORD*)>(GetProcAddress(dll,"_BinkSetSoundTrack@8"));
            Check(open&&close&&id&&set,"actual stdcall codec exports resolved");
            if(open&&close&&id&&set){
                char file[MAX_PATH];sprintf_s(file,"%s\\MA2_5500.bik",argv[1]);
                std::ifstream input(file,std::ios::binary);
                std::vector<char> movie((std::istreambuf_iterator<char>(input)),std::istreambuf_iterator<char>());
                const char* found=movie.empty()?nullptr:catalog.Identify(movie.data(),0x04004400);
                const std::uint32_t tracks[]{0,1,2,3};const auto plan=BinkPlanIntroAudio(found,true,tracks,4,0x04004400);
                unsigned opens=0;
                void* handle=BinkOpenWithIntroPlan(plan,[&](std::uint32_t n,const std::uint32_t* a){DWORD native[8]{};for(unsigned i=0;i<n;++i)native[i]=a[i];set(n,native);},[&](){++opens;return open(movie.data(),0x04004400);});
                Check(plan.enabled && handle && opens==1,"actual codec memory Open uses production selection sequence exactly once");
                if(handle){Check(id(handle,8)==10 && id(handle,6)==8,"actual native GetTrackID proves ordinal/ID distinction");close(handle);}
            }
            FreeLibrary(dll);
        }
    }
    return failures?1:0;
}
