#include "native_registry_policy.h"
namespace NativeRegistry {
static uintptr_t Find(const char* symbol){
    const auto image=(uintptr_t)GetModuleHandleA(nullptr);
    if(!PauseRttProbe::SafeRange(image,sizeof(IMAGE_DOS_HEADER)))return 0;
    const auto dos=(const IMAGE_DOS_HEADER*)image;
    if(dos->e_magic!=IMAGE_DOS_SIGNATURE||dos->e_lfanew<=0||dos->e_lfanew>4096)return 0;
    const auto nt=(const IMAGE_NT_HEADERS32*)(image+dos->e_lfanew);
    if(!PauseRttProbe::SafeRange((uintptr_t)nt,sizeof(*nt))||nt->Signature!=IMAGE_NT_SIGNATURE||
       nt->FileHeader.Machine!=IMAGE_FILE_MACHINE_I386||nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR32_MAGIC||
       !nt->FileHeader.NumberOfSections||nt->FileHeader.NumberOfSections>32||
       nt->OptionalHeader.SizeOfImage>128u*1024u*1024u)return 0;
    const auto sections=IMAGE_FIRST_SECTION(nt);
    if(!PauseRttProbe::SafeRange((uintptr_t)sections,nt->FileHeader.NumberOfSections*sizeof(*sections)))return 0;
    nativeregistry::Region ranges[96]{};unsigned used=0;
    for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){const auto& s=sections[i];
        if(!(s.Characteristics&IMAGE_SCN_MEM_READ)||(s.Characteristics&IMAGE_SCN_MEM_EXECUTE))continue;
        const auto size=s.Misc.VirtualSize,rva=s.VirtualAddress;
        if(!size)continue;
        if(rva>nt->OptionalHeader.SizeOfImage||size>nt->OptionalHeader.SizeOfImage-rva)return 0;
        // VirtualProtect may split one image section into several regions.
        // Enumerate them instead of incorrectly treating the entire section as
        // unreadable when it crosses a protection boundary.
        uintptr_t at=image+rva;const auto end=at+size;
        while(at<end){
            MEMORY_BASIC_INFORMATION info{};
            if(!VirtualQuery((void*)at,&info,sizeof(info)))return 0;
            const auto regionEnd=(uintptr_t)info.BaseAddress+info.RegionSize;
            if(regionEnd<=at)return 0;
            const auto stop=regionEnd<end?regionEnd:end;
            if(PauseRttProbe::SafeRange(at,stop-at)){
                if(used>=96)return 0;
                ranges[used++]={(const unsigned char*)at,stop-at,(std::uint32_t)at};
            }
            at=stop;
        }
    }
    return nativeregistry::Find(ranges,used,symbol);
}
}
