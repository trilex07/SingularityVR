#include "../scope_pip_policy.h"
#include "../weapon_muzzle_geometry.h"
#include <cassert>
#include <array>
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
    scopepip::PresentationBudget budget;
    for(unsigned n=0;n<200;++n)assert(budget.Observe(false,false));
    for(unsigned n=0;n<29;++n)assert(budget.Observe(true,false));
    assert(budget.Observe(true,true)&&budget.missed==0);
    for(unsigned n=0;n<29;++n)assert(budget.Observe(true,false));
    assert(!budget.Observe(true,false)&&budget.fault);
    assert(!budget.Observe(true,true)); // explicit restart, no endless retries
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
    // Native half-frame camera matrices already encode eye remapping. Do not
    // apply UI half-width conversion again. Point movement changes placement.
    for(unsigned eye=0;eye<2;++eye){
        const float clip[4]{eye?1.f:-1.f,0,1,2};
        assert(scopepip::ClipPixel(6488,3308,clip,pixel));
        assert(pixel.x==(eye+.5f)*3244&&pixel.y==1654&&pixel.rhw==.5f);
        const float shifted[4]{clip[0]+.1f,.2f,1,2};
        scopepip::Pixel moved{};assert(scopepip::ClipPixel(6488,3308,shifted,moved));
        assert(moved.x>pixel.x&&moved.y<pixel.y);
    }
    for(const auto& clip: {std::array<float,4>{0,0,1,0},std::array<float,4>{nan,0,1,1},
                          std::array<float,4>{0,0,1,-1}})
        assert(!scopepip::ClipPixel(6488,3308,clip.data(),pixel));
    const float clip[4]{0,0,1,1};
    assert(!scopepip::ClipPixel(6487,3308,clip,pixel));
    scopepip::Aperture aperture{};aperture.pawn=1;aperture.weapon=2;aperture.tick=100;
    aperture.width=6488;aperture.height=3308;
    assert(aperture.Current(1,2,250,6488,3308));
    assert(!aperture.Current(1,2,251,6488,3308));
    assert(!aperture.Current(1,3,200,6488,3308));
    assert(!aperture.Current(1,2,99,6488,3308));
    assert(!aperture.Current(1,2,200,6488,2688));
    unsigned depth=0;
    {scopepip::RenderLease off(depth,false,1,1);assert(!depth);}
    {scopepip::RenderLease foreign(depth,true,1,2);assert(!depth);}
    {scopepip::RenderLease unknown(depth,true,0,0);assert(!depth);}
    {scopepip::RenderLease own(depth,true,1,1);assert(depth==1);
        {scopepip::RenderLease nested(depth,true,1,1);assert(depth==2);}
        assert(depth==1);}
    assert(!depth);
}
