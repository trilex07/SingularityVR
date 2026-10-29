#define NOMINMAX
#include "../volk_mechanics.h"
#include <cassert>
#include <fstream>
#include <iterator>
#include <cstdio>
#include <vector>
int main(int argc,char** argv){
    assert(argc==2);std::ifstream in(argv[1],std::ios::binary);std::vector<unsigned char> b{std::istreambuf_iterator<char>(in),{}};
    weaponreloadanim::Data d;assert(weaponreloadanim::Decode(b.data(),b.size(),weaponreload::Kind::Volk,d));assert(weaponreloadanim::ToNative(d));
    armbind::Mat end;assert(volkmechanics::Relative(d,0,74,74,end));
    float maximum=0;for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)maximum=std::max(maximum,fabsf(end.m[r][c]-(r==c?1.f:0.f)));assert(maximum<.00003f);
    // Two controller roots, 1 ms sweep through eject/carry/native ack/seat.
    for(unsigned pose=0;pose<2;++pose){
        float parent[60],out[60];for(unsigned slot=0;slot<5;++slot){armbind::Mat m{};float a=pose*.83f;m.m[0][0]=m.m[1][1]=cosf(a);m.m[0][1]=sinf(a);m.m[1][0]=-sinf(a);m.m[2][2]=m.m[3][3]=1;m.m[3][0]=slot+pose*8.f;armbind::ToGpu(m,reinterpret_cast<float(*)[4]>(parent+slot*12));}
        for(unsigned ms=0;ms<=1500;++ms){
            auto phase=ms<500?weaponreload::Phase::NeedAmmo:ms<1000?weaponreload::Phase::Carrying:weaponreload::Phase::WaitingNative;
            assert(volkmechanics::Palette(d,phase,1000,ms>=1000?2000:0,1000+ms,parent,15,out));
            assert(!memcmp(parent+24,out+24,36*sizeof(float))); // root, static release, trigger
            for(float f:out)assert(std::isfinite(f));
            if(ms<1000)for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)assert(out[r*4+c]==0);
            if(ms>=1167)assert(!memcmp(parent,out,sizeof(parent))); // no tail or baked root
        }
    }
    d.kind=weaponreload::Kind::Valkyrie;float a[60]{},out[60]{};assert(!volkmechanics::Palette(d,weaponreload::Phase::NeedAmmo,1,0,1,a,15,out));
    printf("Volk mechanics: endpoint matrix max-abs error %.9g; 3002 timeline/controller samples PASS\n",maximum);
}

