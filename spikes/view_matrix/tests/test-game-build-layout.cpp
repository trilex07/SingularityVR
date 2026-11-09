#include "../game_build_layout.h"
#include "../game_build_input.h"
#include <cassert>
#include <cstdio>
#include <vector>
#include <fstream>
#include <iterator>
#include <string>
#include <windows.h>

static std::vector<unsigned char> Image(const char* path){
    std::ifstream file(path,std::ios::binary);assert(file);
    std::vector<unsigned char> raw((std::istreambuf_iterator<char>(file)),{});
    assert(raw.size()>=sizeof(IMAGE_DOS_HEADER));
    auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(raw.data());
    assert(dos->e_magic==IMAGE_DOS_SIGNATURE&&dos->e_lfanew>0&&unsigned(dos->e_lfanew)+sizeof(IMAGE_NT_HEADERS32)<=raw.size());
    auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(raw.data()+dos->e_lfanew);
    assert(nt->Signature==IMAGE_NT_SIGNATURE&&nt->FileHeader.Machine==IMAGE_FILE_MACHINE_I386&&nt->OptionalHeader.Magic==IMAGE_NT_OPTIONAL_HDR32_MAGIC);
    assert(nt->OptionalHeader.ImageBase==0x400000&&nt->OptionalHeader.SizeOfImage<64*1024*1024);
    // GOG's English header records an unaligned SizeOfImage. Account for the
    // final mapped page; this fixture never executes or changes the image.
    std::vector<unsigned char> image((nt->OptionalHeader.SizeOfImage+4095u)&~4095u);
    auto sec=IMAGE_FIRST_SECTION(nt);
    for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){
        assert(sec[i].PointerToRawData+sec[i].SizeOfRawData<=raw.size());
        assert(sec[i].VirtualAddress+sec[i].SizeOfRawData<=image.size());
        std::memcpy(image.data()+sec[i].VirtualAddress,raw.data()+sec[i].PointerToRawData,sec[i].SizeOfRawData);
    }
    return image;
}
int main(int argc,char** argv){
    assert(argc==3);auto image=Image(argv[1]);
    auto read=[&](uintptr_t address,unsigned n)->const unsigned char*{
        if(address<0x400000||address-0x400000>image.size()||n>image.size()-(address-0x400000))return nullptr;
        return image.data()+address-0x400000;
    };
    auto selected=gamebuild::Select(read);
    if(std::string(argv[2])=="unsupported"){assert(!selected);puts("PASS unsupported disk image refused (protected disk code is not a runtime verdict)");return 0;}
    assert(selected&&selected->name==std::string(argv[2]));
    const auto original=*selected;
    assert(gamebuild::RuntimeAllowed(original)==(std::string(argv[2])=="english-map"));
    assert(gamebuild::Matches(original,read));
    const auto& input=gamebuild::InputFor(original);
    assert(gamebuild::MatchesInput(input,read));
    auto wrongSlot=input;wrongSlot.mouseSlot+=4;assert(!gamebuild::MatchesInput(wrongSlot,read));
    auto wrongIdentity=input;wrongIdentity.padIdentity+=4;assert(!gamebuild::MatchesInput(wrongIdentity,read));
    auto deniedInput=[&](uintptr_t address,unsigned n)->const unsigned char*{return address==input.engine?nullptr:read(address,n);};
    assert(!gamebuild::MatchesInput(input,deniedInput));
    assert(!gamebuild::MatchesInput(gamebuild::InputFor(std::string(argv[2])=="english-map"?gamebuild::Alternate():gamebuild::English()),read));
    for(uintptr_t address:{input.mouse[0],input.mouse[1],input.mouse[2],input.mouse[3],input.padCallback,
        input.cursor[0],input.cursor[1],input.cursor[2],input.start,input.stop,input.poll,input.padEnum,
        input.refs[0],input.refs[1],input.refs[2],input.refs[3],input.refs[4]}){
        image[address-0x400000]^=1;assert(!gamebuild::MatchesInput(input,read));image[address-0x400000]^=1;
    }
    // Individually corrupt each identity/operand gate and require rejection.
    for(uintptr_t address:{original.calc,original.line,original.single,original.single+33,uintptr_t(0x48bce0),uintptr_t(0x42fcb5),uintptr_t(0x417ed8)}){
        image[address-0x400000]^=1;assert(!gamebuild::Select(read));image[address-0x400000]^=1;
    }
    auto denied=[&](uintptr_t address,unsigned n)->const unsigned char*{return address==original.objects?nullptr:read(address,n);};
    assert(!gamebuild::Select(denied));
    gamebuild::Current()=original;assert(gamebuild::Current().single==original.single);
    assert(gamebuild::Input().padCallback==input.padCallback);
    printf("PASS %s: real PE core + input contracts; gate mutations, mixed profiles and unreadable roots refused; no execution\n",original.name);
}
