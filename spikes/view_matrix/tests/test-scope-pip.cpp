#include "../scope_pip_policy.h"
#include "../weapon_muzzle_geometry.h"
#include <cassert>
#include <limits>
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
    // Each eye's centre lands at 1/4 or 3/4 of the final SBS backbuffer,
    // not at the centre of a stale offscreen viewport or the opposite eye.
    for(unsigned eye=0;eye<2;++eye){
        scopepip::Pixel pixel{};
        assert(scopepip::EyePixel(eye,6488,3308,{.5f,.5f,.2f},pixel));
        assert(pixel.x==(eye+.5f)*3244&&pixel.y==1654&&pixel.rhw==5);
        uipointer::ScreenPoint p{};
        const float ex=eye?.032f:-.032f;
        assert(uipointer::ProjectPoint({ex,0,-.2f},{ex,0,0},{0,0,-1},{1,0,0},{0,1,0},1,1,p));
        assert(scopepip::EyePixel(eye,6488,3308,p,pixel));
        assert(pixel.x==(eye+.5f)*3244);
    }
    scopepip::Pixel pixel{1,2,3};
    const auto nan=std::numeric_limits<float>::quiet_NaN();
    assert(!scopepip::EyePixel(2,6488,3308,{.5f,.5f,.2f},pixel));
    assert(!scopepip::EyePixel(0,6487,3308,{.5f,.5f,.2f},pixel));
    assert(!scopepip::EyePixel(0,6488,0,{.5f,.5f,.2f},pixel));
    assert(!scopepip::EyePixel(0,6488,3308,{nan,.5f,.2f},pixel));
    assert(!scopepip::EyePixel(0,6488,3308,{.5f,.5f,0},pixel));
    assert(!scopepip::EyePixel(0,6488,3308,{.5f,.5f,-.2f},pixel));
    assert(pixel.x==1&&pixel.y==2&&pixel.rhw==3);
    unsigned depth=0;
    {scopepip::RenderLease off(depth,false,1,1);assert(!depth);}
    {scopepip::RenderLease foreign(depth,true,1,2);assert(!depth);}
    {scopepip::RenderLease unknown(depth,true,0,0);assert(!depth);}
    {scopepip::RenderLease own(depth,true,1,1);assert(depth==1);
        {scopepip::RenderLease nested(depth,true,1,1);assert(depth==2);}
        assert(depth==1);}
    assert(!depth);
}
