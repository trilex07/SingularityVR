#include "../hand_anchor_math.h"
#include <cassert>
#include <limits>
#include <cstdio>
static void identity(float m[4][4]){for(int r=0;r<4;++r)for(int c=0;c<4;++c)m[r][c]=r==c?1.f:0.f;}
static bool close(armik::V a,armik::V b){return armik::Length(a-b)<.001f;}
int main(){
    assert(handanchor::PistolGrip(true,true,false,5799,false));
    assert(!handanchor::PistolGrip(true,false,false,5799,false)); // cached mesh is not an equipped weapon
    assert(!handanchor::PistolGrip(true,true,true,5799,false));
    assert(!handanchor::PistolGrip(true,true,false,5799,true));
    assert(!handanchor::PistolGrip(true,true,false,6100,false));
    assert(!handanchor::PistolGrip(false,true,false,5799,false));
    armbind::Mat bind;identity(bind.m);bind.m[3][0]=12;bind.m[3][1]=-7;bind.m[3][2]=3;
    float skin[3][4]={{1,0,0,-12},{0,1,0,7},{0,0,1,0}}; // animated joint, not skin translation
    float local[4][4];identity(local);local[3][0]=20;local[3][2]=-40;
    float controller[4][4]={{0,.75f,0,0},{-.75f,0,0,0},{0,0,.75f,0},{10,8,-4,1}};
    const armik::V target{5,2,-6};float adjusted[4][4];
    assert(handanchor::TargetWrist(adjusted,bind,skin,local,controller,target));
    armbind::Mat joint;assert(armbind::CurrentJoint(bind,skin,joint));
    auto native=handanchor::Point(armbind::Position(joint),local);
    assert(armik::Length(handanchor::Point(native,controller)-target)>20);
    assert(close(handanchor::Point(native,adjusted),target));
    // Pistol selection must not undo free-hand wrist correction. The stale
    // controller target reproduces the reported offset; the selected final
    // target reaches the tracked wrist for every actual camera-ride case.
    assert(handanchor::AimTarget(false,controller,adjusted)==adjusted);
    assert(handanchor::AimTarget(true,controller,adjusted)==controller);
    for(int r=0;r<3;++r)for(int c=0;c<4;++c)assert(adjusted[r][c]==controller[r][c]);
    // The actual camera ride must cancel, even when it differs from the live target (holster/ADS).
    for(int moved=0;moved<3;++moved){
        float ride[4][4];identity(ride);
        if(moved){ride[3][0]=100.f*moved;ride[0][0]=ride[1][1]=ride[2][2]=.5f;}
        float deltaL[4][4],deltaR[4][4],posed[3][4];
        assert(skeletal::LocalDeltaPair(deltaL,deltaR,local,ride,adjusted,
            handanchor::AimTarget(false,controller,adjusted)));
        skeletal::TransformBone(posed,skin,deltaR);
        assert(armbind::CurrentJoint(bind,posed,joint));
        assert(close(handanchor::Point(handanchor::Point(armbind::Position(joint),local),ride),target));
    }
    float sentinel[4][4];identity(sentinel);local[0][0]=std::numeric_limits<float>::quiet_NaN();
    assert(!handanchor::TargetWrist(sentinel,bind,skin,local,controller,target));assert(sentinel[3][0]==0);
    for(int enabled=0;enabled<=1;++enabled)for(int cine=0;cine<=1;++cine)for(int menu=0;menu<=1;++menu)
    for(int paused=0;paused<=1;++paused)for(int game=0;game<=1;++game)
        assert(handanchor::GameplaySource(enabled,cine,menu,paused,game)==bool(enabled&&!cine&&!menu&&!paused&&game));
    std::puts("PASS: reconstructed wrist target, preserved rotation/scale, actual ride cancellation, fail-closed, gameplay-only gate");
}
