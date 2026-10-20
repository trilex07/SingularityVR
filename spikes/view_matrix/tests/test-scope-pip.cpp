#include "../scope_pip_policy.h"
#include "../weapon_muzzle_geometry.h"
#include <cassert>
int main(){
    using hipknife::V;
    for(int n=0;n<360;++n){
        const float yaw=n*.0174532925f,base=(359-n)*.0174532925f;
        const V p{17,-23,9},centre{.12f,1.8f,-.7f};V tracking{},back{};
        assert(weaponmuzzle::ToTracking(p,centre,yaw,base,50.f,tracking));
        assert(scopepip::Relative(tracking,centre,yaw,base,50.f,back));
        assert(hipknife::Length(back-p)<.0001f);
    }
    V right{},up{};assert(scopepip::Basis({0,0,-1},{0,1,0},right,up));
    assert(hipknife::Length(right-V{1,0,0})<.0001f);
    assert(!scopepip::Basis({0,0,-1},{0,0,-2},right,up));
    int pitch=0,yaw=0,roll=0;
    assert(scopepip::Angles({1,0,0},{0,0,1},pitch,yaw,roll));
    assert(pitch==0&&yaw==0&&roll==0);
    assert(scopepip::Angles({0,0,1},{-1,0,0},pitch,yaw,roll));assert(pitch==16384);
    assert(scopepip::Angles({0,0,-1},{1,0,0},pitch,yaw,roll));assert(pitch==-16384);
    assert(!scopepip::Angles({1,0,0},{2,0,0},pitch,yaw,roll));
    scopepip::Frame f{true,1,2,100,{},{0,0,-1},{0,1,0},.016f,0};
    assert(f.Current(1,2,250));assert(!f.Current(1,2,251));
    assert(!f.Current(2,2,100));assert(!f.Current(1,2,99));
    f.eye=-1;assert(!f.Current(1,2,100));f.eye=2;assert(!f.Current(1,2,100));f.eye=0;
    scopepip::Completion c;c.Bind(true);c.Bind(false);assert(!c.epoch);
    c.Bind(true);c.Draw(false);c.Bind(false);assert(!c.epoch);
    c.Bind(true);c.Draw(true);assert(!c.epoch);c.Bind(false);assert(c.epoch==1);
    c.Reset();assert(!c.bound&&!c.draws&&!c.epoch);
}
