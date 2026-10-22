#define NOMINMAX
#include "../tmd_short_gauntlet.h"
#include <cassert>
#include <cstring>
#include <vector>

int main() {
    std::vector<unsigned char> bytes(24 + 7110 * 6);
    auto put=[](unsigned char* p,unsigned x){std::memcpy(p,&x,4);};
    std::memcpy(bytes.data(),"SVTG",4);
    put(bytes.data()+4,1);put(bytes.data()+8,7444);put(bytes.data()+12,7110);
    put(bytes.data()+16,23);put(bytes.data()+20,36369);
    for (unsigned i=0;i<7110;++i) {
        const std::uint16_t tri[3]={0,1,2};
        std::memcpy(bytes.data()+24+i*6,tri,6);
    }
    tmdgauntlet::Data data;
    assert(tmdgauntlet::Decode(bytes.data(),bytes.size(),data));
    assert(data.indices.size()==7110*3 && data.capTriangles==23);
    assert(!tmdgauntlet::Decode(bytes.data(),bytes.size()-1,data));
    bytes[4]=2;assert(!tmdgauntlet::Decode(bytes.data(),bytes.size(),data));bytes[4]=1;
    std::uint16_t bad=7444;std::memcpy(bytes.data()+24,&bad,2);
    assert(!tmdgauntlet::Decode(bytes.data(),bytes.size(),data));
    assert(tmdgauntlet::Eligible(true,true,true,true,2,true,false,false));
    assert(!tmdgauntlet::Eligible(false,true,true,true,2,true,false,false));
    assert(!tmdgauntlet::Eligible(true,true,false,true,2,true,false,false));
    assert(!tmdgauntlet::Eligible(true,true,true,true,2,true,true,false));
    assert(!tmdgauntlet::Eligible(true,true,true,true,2,true,false,true));
    assert(tmdgauntlet::IsRightForearmTail(7444,36318,17));
    assert(!tmdgauntlet::IsRightForearmTail(7444,13206,7704));
    return 0;
}
