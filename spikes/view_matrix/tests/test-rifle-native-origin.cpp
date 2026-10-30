#include "../native_rifle_fx_policy.h"
#include <cassert>
#include <limits>
int main(){
    using hipknife::V;
    weaponmuzzle::Witness w{true,weaponreload::Kind::Valkyrie,1,2,100,{5,6,7}};
    V out{99,99,99};
    assert(riflefx::Origin(2,true,true,2,1,2,110,w,{100,200,300},{100,200,300},out));
    assert(out.x==105&&out.y==206&&out.z==307);
    assert(riflefx::Origin(3,true,true,2,1,2,110,w,{100,200,300},{100,200,300},out));
    assert(!riflefx::Origin(4,true,true,2,1,2,110,w,{}, {},out));
    assert(riflefx::Lease(3,true,100,250,1,2,weaponmuzzle::Witness{true,weaponreload::Kind::Valkyrie,1,2,250,{}}));
    assert(!riflefx::Lease(3,true,100,251,1,2,weaponmuzzle::Witness{true,weaponreload::Kind::Valkyrie,1,2,251,{}}));
    assert(!riflefx::Lease(2,true,100,110,1,2,w));
    assert(!riflefx::Lease(3,false,100,110,1,2,w));
    assert(!riflefx::Lease(3,true,111,110,1,2,w));
    assert(!riflefx::Lease(3,true,100,110,3,2,w));
    assert(!riflefx::Lease(3,true,100,110,1,3,w));
    int pitch=0,yaw=0;
    assert(riflefx::Rotation({1,0,0},pitch,yaw)&&pitch==0&&yaw==0);
    assert(riflefx::Rotation({0,1,0},pitch,yaw)&&pitch==0&&yaw==16384);
    assert(riflefx::Rotation({0,0,1},pitch,yaw)&&pitch==16384);
    assert(riflefx::Rotation({0,0,-1},pitch,yaw)&&pitch==-16384);
    assert(!riflefx::Rotation({},pitch,yaw));
    assert(!riflefx::Rotation({2,0,0},pitch,yaw));
    assert(!riflefx::Rotation({std::numeric_limits<float>::quiet_NaN(),0,0},pitch,yaw));
    for(int mode=0;mode<2;++mode)assert(!riflefx::Origin(mode,true,true,2,1,2,110,w,{}, {},out));
    assert(!riflefx::Origin(2,false,true,2,1,2,110,w,{}, {},out));
    assert(!riflefx::Origin(2,true,false,2,1,2,110,w,{}, {},out));
    assert(!riflefx::Origin(2,true,true,3,1,2,110,w,{}, {},out));
    assert(!riflefx::Origin(2,true,true,2,3,2,110,w,{}, {},out));
    assert(!riflefx::Origin(2,true,true,2,1,2,251,w,{}, {},out));
    assert(!riflefx::Origin(2,true,true,2,1,2,99,w,{}, {},out));
    assert(!riflefx::Origin(2,true,true,2,1,2,110,w,{10000,0,0}, {},out));
    assert(!riflefx::Origin(2,true,true,2,1,2,110,w,{std::numeric_limits<float>::quiet_NaN(),0,0}, {},out));
    assert(riflefx::TracerSource(true,true,true,2+0x2d4,1,2,110,w,{100,200,300},{100,200,300},out));
    assert(out.x==105&&out.y==206&&out.z==307);
    assert(!riflefx::TracerSource(false,true,true,2+0x2d4,1,2,110,w,{}, {},out));
    assert(!riflefx::TracerSource(true,false,true,2+0x2d4,1,2,110,w,{}, {},out));
    assert(!riflefx::TracerSource(true,true,false,2+0x2d4,1,2,110,w,{}, {},out));
    assert(!riflefx::TracerSource(true,true,true,3+0x2d4,1,2,110,w,{}, {},out));
    assert(!riflefx::TracerSource(true,true,true,2+0x2d4,1,2,251,w,{}, {},out));
    // Same first-person spawn contract; loop/tracer paths remain rifle-only.
    for(const unsigned mesh:{11389u,5799u}){
        w.kind=mesh==11389?weaponreload::Kind::Kasimov:weaponreload::Kind::None;
        assert(!riflefx::Origin(3,true,true,2,1,2,110,w,{}, {},out));
        assert(riflefx::Origin(3,true,true,2,1,2,110,w,{}, {},out,true,mesh));
        assert(!riflefx::Origin(3,true,true,2,1,2,110,w,{}, {},out,true,8297));
        assert(!riflefx::Origin(3,true,false,2,1,2,110,w,{}, {},out,true,mesh));
        assert(!riflefx::Origin(3,true,true,3,1,2,110,w,{}, {},out,true,mesh));
        assert(!riflefx::Origin(3,true,true,2,1,2,251,w,{}, {},out,true,mesh));
        assert(!riflefx::Lease(3,true,100,110,1,2,w));
        assert(!riflefx::TracerSource(true,true,true,2+0x2d4,1,2,110,w,{}, {},out));
    }
    w.kind=weaponreload::Kind::Kasimov;
    assert(!riflefx::Origin(2,true,true,2,1,2,110,w,{}, {},out));
}
