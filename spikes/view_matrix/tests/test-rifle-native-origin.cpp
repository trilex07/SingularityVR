#include "../native_rifle_fx_policy.h"
#include <cassert>
#include <limits>
int main(){
    using hipknife::V;
    weaponmuzzle::Witness w{true,weaponreload::Kind::Valkyrie,1,2,100,{5,6,7}};
    V out{99,99,99};
    assert(riflefx::Origin(2,true,true,2,1,2,110,w,{100,200,300},{100,200,300},out));
    assert(out.x==105&&out.y==206&&out.z==307);
    for(int mode=0;mode<2;++mode)assert(!riflefx::Origin(mode,true,true,2,1,2,110,w,{}, {},out));
    assert(!riflefx::Origin(2,false,true,2,1,2,110,w,{}, {},out));
    assert(!riflefx::Origin(2,true,false,2,1,2,110,w,{}, {},out));
    assert(!riflefx::Origin(2,true,true,3,1,2,110,w,{}, {},out));
    assert(!riflefx::Origin(2,true,true,2,3,2,110,w,{}, {},out));
    assert(!riflefx::Origin(2,true,true,2,1,2,251,w,{}, {},out));
    assert(!riflefx::Origin(2,true,true,2,1,2,99,w,{}, {},out));
    assert(!riflefx::Origin(2,true,true,2,1,2,110,w,{10000,0,0}, {},out));
    assert(!riflefx::Origin(2,true,true,2,1,2,110,w,{std::numeric_limits<float>::quiet_NaN(),0,0}, {},out));
    w.kind=weaponreload::Kind::Kasimov;
    assert(!riflefx::Origin(2,true,true,2,1,2,110,w,{}, {},out));
}
