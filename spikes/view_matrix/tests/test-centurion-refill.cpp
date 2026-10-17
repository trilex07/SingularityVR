#include "../centurion_refill_only.h"
#include <cassert>
#include <cstdio>
#include <array>
#include <fstream>
#include <cstdlib>
#include <vector>
using namespace centurionrefill;
struct Fake {
    Counts counts;int calls=0;bool bad=false;
    Counts Read()const{return counts;}
    void Refill(){++calls;const int n=std::min(counts.capacity-counts.clip,counts.reserve);
        counts.clip+=n;if(!bad)counts.reserve-=n;}
};
int main(int argc,char** argv){
    if(argc==7){
        const auto base=std::strtoul(argv[2],nullptr,0);
        std::ifstream in(argv[1],std::ios::binary);assert(in.good());
        in.seekg(0,std::ios::end);const auto n=in.tellg();assert(n>0&&n<64*1024*1024);
        for(unsigned j=0;j<4;++j){
            const auto va=std::strtoul(argv[j+3],nullptr,0);const auto& s=Specs[j];
            assert(va>=base&&va-base+s.bytes<=(unsigned long)n);
            std::vector<std::uint8_t> raw(s.bytes);
            in.seekg(va-base);in.read((char*)raw.data(),s.bytes);assert(in.good());
            assert(Audited(raw.data(),raw.size(),s));
            for(std::size_t i=0;i<raw.size();++i){
                auto bad=raw;bad[i]^=1;
                assert(Audited(bad.data(),bad.size(),s)==(i>=s.rel&&i<s.rel+4));
            }
            assert(!Audited(raw.data(),raw.size()-1,s));
        }
        std::puts("PASS archived ORIGINAL four entire methods + every-byte mutations (not runtime)");
    }else assert(argc==1);
    for(int clip=0;clip<=6;++clip)for(int reserve=0;reserve<100;++reserve){
        const Counts before{clip,reserve,6};Fake f{before};const auto r=Commit(before,f);
        const int moved=std::min(6-clip,reserve);
        assert(f.calls==(moved?1:0));
        assert(r==(moved?Result::Conserved:Result::Rejected));
        assert((f.counts==Counts{clip+moved,reserve-moved,6}));
        if(moved){assert(Commit(before,f)==Result::Changed);assert(f.calls==1);}
    }
    for(const Counts invalid:std::array<Counts,7>{{{-1,6,6},{7,6,6},{0,-1,6},{0,1,5},
        {0,1,7},{0,100001,6},{0,1,-1}}}){
        Fake f{invalid};assert(Commit(invalid,f)==Result::Rejected&&f.calls==0);
    }
    Fake changed{{2,8,6}};assert(Commit({1,8,6},changed)==Result::Changed&&changed.calls==0);
    Fake fault{{0,10,6},0,true};assert(Commit(fault.counts,fault)==Result::Fault&&fault.calls==1);
    centurionnative::Gate gate;gate.Publish({true,2,1,1000,true,0,10,6});
    assert(gate.Arm(2,1,1000));gate.Publish({true,2,1,1010,true,1,9,6});
    assert(gate.permitted.clip==0&&gate.permitted.reserve==10);
    assert(gate.Evaluate(2,1,2,1,1010)==centurionnative::Decision::Permit);
    assert(gate.Evaluate(2,1,2,1,1011)==centurionnative::Decision::Block);
    gate.Publish({true,2,1,1020,false,0,10,6});assert(!gate.Arm(2,1,1020));
    std::puts("PASS refill policy: finite/partial/empty reserves, snapshot race, conservation, one-shot, holster");
}
