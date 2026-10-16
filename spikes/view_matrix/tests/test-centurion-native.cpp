#include "../centurion_native_reload.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <fstream>
#include <cstdlib>
using namespace centurionnative;
int main(int argc,char** argv){
    if(argc==5){
        // Read-only comparison with archived original bytes. Addresses are
        // arguments from the offline mapper, not hardcoded runtime locations.
        const auto base=std::strtoul(argv[2],nullptr,0);
        const auto wrapperVa=std::strtoul(argv[3],nullptr,0),methodVa=std::strtoul(argv[4],nullptr,0);
        std::ifstream in(argv[1],std::ios::binary);assert(in.good());
        in.seekg(0,std::ios::end);const auto bytes=in.tellg();
        assert(bytes>0&&bytes<=64*1024*1024&&wrapperVa>=base&&methodVa>=base);
        assert(wrapperVa-base+WrapperBytes<=(unsigned long)bytes&&methodVa-base+MethodBytes<=(unsigned long)bytes);
        std::array<std::uint8_t,WrapperBytes> w{};std::array<std::uint8_t,MethodBytes> m{};
        in.seekg(wrapperVa-base);in.read((char*)w.data(),w.size());assert(in.good());
        in.seekg(methodVa-base);in.read((char*)m.data(),m.size());assert(in.good());
        std::uint32_t slot=0;assert(DecodeWrapper(w.data(),w.size(),slot)&&AuditedMethod(m.data(),m.size()));
        std::printf("PASS archived ORIGINAL native ABI/body comparison: derived vslot=0x%X (not runtime proof)\n",slot);
    }else assert(argc==1);
    // Synthetic wrapper fixture (not extracted game bytes). Mutation of every
    // non-relocation byte must refuse the ABI, not blindly trust an object name.
    std::array<std::uint8_t,WrapperBytes> wrapper{};
    const std::uint8_t prefix[]={0x55,0x8b,0xec,0x8b,0x45,0x08,0xff,0x40,
        0x18,0x56,0x8b,0xf1,0x8b,0x48,0x18,0x80,0x39,0x41,0x75,0x10,
        0x41,0x6a,0,0x89,0x48,0x18,0x8b,0x48,0x14,0x50,0xff,0x15};
    const std::uint8_t dispatch[]={0x8b,0x06,0x8b,0x90};
    const std::uint8_t tail[]={0x8b,0xce,0xff,0xd2,0x8b,0x4d,0x0c,0x89,0x01,0x5e,0x5d,0xc2,8,0};
    std::memcpy(wrapper.data(),prefix,sizeof(prefix));std::memcpy(wrapper.data()+36,dispatch,sizeof(dispatch));
    std::memcpy(wrapper.data()+44,tail,sizeof(tail));
    std::uint32_t v=0x4c4,slot=0;std::memcpy(wrapper.data()+40,&v,4);
    assert(DecodeWrapper(wrapper.data(),wrapper.size(),slot)&&slot==v);
    for(std::size_t i=0;i<wrapper.size();++i){
        auto bad=wrapper;bad[i]^=0x01;
        if((i>=32&&i<36)||(i>=40&&i<44))continue;
        assert(!DecodeWrapper(bad.data(),bad.size(),slot));
    }
    for(std::uint32_t invalid:{0u,0x4c5u,0x2000u,0xfffffffcu}){
        std::memcpy(wrapper.data()+40,&invalid,4);assert(!DecodeWrapper(wrapper.data(),wrapper.size(),slot));}
    assert(!DecodeWrapper(nullptr,100,slot)&&!DecodeWrapper(wrapper.data(),57,slot));
    std::array<std::uint8_t,MethodBytes> method{};
    const std::uint8_t head[]={0x55,0x8b,0xec,0x83,0xec,8,0x8b,0x15};
    const std::uint8_t locals[]={0x53,0x56,0x57,0x33,0xdb,0x53,0x8d,0x45,0xfc,0x50,0x8b,0xf1};
    const std::uint8_t counts[]={0x3b,0x5d,0xfc,0x7d,0x40,0x85,0xc0,0x7e,0x3c,0x8b,0xce,0xe8};
    const std::uint8_t end[]={0xc7,0x45,0xf8,1,0,0,0,0x8b,0x45,0xf8,0x5f,0x5e,0x5b,0x8b,0xe5,0x5d,0xc3};
    std::memcpy(method.data(),head,sizeof(head));std::memcpy(method.data()+12,locals,sizeof(locals));
    std::memcpy(method.data()+0x184,counts,sizeof(counts));std::memcpy(method.data()+0x1c2,end,sizeof(end));
    assert(AuditedMethod(method.data(),method.size()));
    for(auto i:{0u,5u,12u,23u,0x184u,0x18bu,0x1c2u,0x1d2u}){
        auto bad=method;bad[i]^=1;assert(!AuditedMethod(bad.data(),bad.size()));}
    assert(!AuditedMethod(method.data(),method.size()-1));
    Gate g;g.Publish({true,20,10,1000});
    assert(g.Evaluate(0,10,20,10,1000)==Decision::Forward);
    assert(g.Evaluate(1,10,20,10,1000)==Decision::Observe);
    assert(g.Evaluate(2,10,20,10,1000)==Decision::Block); // automatic, no close token
    assert(g.Evaluate(2,11,20,10,1000)==Decision::Forward); // NPC/other gun unchanged
    assert(g.Evaluate(2,10,21,10,1000)==Decision::Forward); // changed pawn
    assert(g.Evaluate(2,10,20,10,999)==Decision::Forward);
    assert(g.Evaluate(2,10,20,10,1251)==Decision::Forward); // expired context fails open
    assert(!g.Arm(20,11,1000));assert(g.Arm(20,10,1000));
    assert(g.Evaluate(1,10,20,10,1000)==Decision::Observe); // read-only does not consume
    assert(g.Evaluate(2,10,20,10,1001)==Decision::Permit);
    assert(g.Evaluate(2,10,20,10,1002)==Decision::Block); // nested/repeated attempt
    assert(g.Arm(20,10,1010));g.Publish({true,20,11,1020});
    assert(g.Evaluate(2,11,20,11,1020)==Decision::Block);
    g.Publish({true,20,10,1100});assert(g.Arm(20,10,1100));
    g.Publish({true,20,10,1550});assert(g.Evaluate(2,10,20,10,1601)==Decision::Block);
    assert(g.Arm(20,10,1560));g.Clear();assert(g.Evaluate(2,10,20,10,1561)==Decision::Forward);
    g.Publish({true,20,10,1700});assert(g.Arm(20,10,1700));
    g.Publish({true,20,10,1951});assert(g.Evaluate(2,10,20,10,1951)==Decision::Block); // context gap revokes token
    for(unsigned run=0;run<100000;++run){
        const std::uint64_t now=2000+run*20;g.Publish({true,20,10,now});
        assert(g.Evaluate(2,11,20,10,now)==Decision::Forward);
        assert(g.Arm(20,10,now));assert(g.Evaluate(2,10,20,10,now)==Decision::Permit);
        assert(g.Evaluate(2,10,20,10,now)==Decision::Block);
    }
    std::puts("Centurion native L1 PASS: wrapper/body rejection, identity/lease/one-shot, 100000 sequences");
}
