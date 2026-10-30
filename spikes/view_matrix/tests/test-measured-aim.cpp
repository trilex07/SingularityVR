#include "../weapon_trace_math.h"
#include "../scope_pip_policy.h"
#include "../weapon_muzzle_geometry.h"
#include <cassert>
#include <limits>
static float dot(const float* a,const float* b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
int main(){
    using K=weaponreload::Kind;
    assert(weaponmuzzle::FinalBody(0,0,11387,0,12834)==K::Valkyrie);
    assert(weaponmuzzle::FinalBody(0,0,11378,0,12834)==K::Valkyrie);
    assert(weaponmuzzle::FinalBody(0,0,11389,0,12942)==K::Kasimov);
    assert(weaponmuzzle::FinalBody(0,0,11387,38502,8)==K::None);
    assert(weaponmuzzle::FinalBody(0,11378,9,38502,8)==K::None);
    assert(weaponmuzzle::FinalBody(0,0,11389,38826,168)==K::None);
    assert(weaponmuzzle::FinalBody(1,0,11387,0,12834)==K::None);
    assert(weaponmuzzle::FinalBody(0,1,11387,0,12834)==K::None);
    assert(weaponmuzzle::FinalBody(0,0,8297,0,10835)==K::None); // accepted Volk unchanged
    assert(weaponmuzzle::FinalBody(0,0,5799,0,6767)==K::None); // different cog slot
    // Reproduce a late TMD correction of the root palette: the witness must
    // equal the rendered vertex chain, not the pre-correction reload snapshot.
    for(auto kind:{K::Valkyrie,K::Kasimov})for(int turn=0;turn<4;++turn){
        weaponreloadanim::Data data;data.ready=true;data.nativeSpace=true;data.kind=kind;
        armbind::Mat before{},after{},local{},controller{};
        for(int j=0;j<4;++j){data.rootBind.m[j][j]=1;before.m[j][j]=1;
            after.m[j][j]=1;local.m[j][j]=1;controller.m[j][j]=1;}
        const float a=turn*1.57079632679f;
        after.m[0][0]=std::cos(a);after.m[0][1]=std::sin(a);
        after.m[1][0]=-std::sin(a);after.m[1][1]=std::cos(a);
        after.m[3][0]=13;after.m[3][1]=-19;after.m[3][2]=7;
        controller.m[3][0]=5;local.m[3][1]=4;
        hipknife::V oldPoint{},finalPoint{},source{};
        assert(weaponmuzzle::Position(data,before,local,controller,oldPoint));
        assert(weaponmuzzle::Position(data,after,local,controller,finalPoint));
        assert(weaponmuzzle::RootPoint(kind,source));
        auto rendered=weaponreload::Transform(weaponreload::Transform(
            weaponreload::Transform(source,after),local),controller);
        assert(hipknife::Length(finalPoint-rendered)<.0001f);
        assert(hipknife::Length(oldPoint-finalPoint)>1.f);
    }
    constexpr float pi=3.14159265359f;
    const float from[]{1,0,0};
    // All headings and near-vertical poses, with a native spread cone. A rigid
    // rotation must retain both its angle and range, including nonzero origin.
    for(int yaw=0;yaw<360;yaw+=5)for(int pitch=-89;pitch<=89;pitch+=7){
        const float y=yaw*pi/180.f,p=pitch*pi/180.f;
        const float to[]{std::cos(p)*std::cos(y),std::cos(p)*std::sin(y),std::sin(p)};
        for(int cone=0;cone<16;++cone){
            const float phi=cone*pi/8.f;
            float start[]{25,-42,13};
            float end[]{start[0]+1000,start[1]+20*std::cos(phi),start[2]+20*std::sin(phi)};
            const float length=std::sqrt(1000000.f+400.f);
            assert(weapontrace::Align(start,end,from,to));
            float ray[]{end[0]-start[0],end[1]-start[1],end[2]-start[2]};
            assert(std::fabs(std::sqrt(dot(ray,ray))-length)<.03f);
            assert(std::fabs(dot(ray,to)-1000.f)<.03f);
            assert(start[0]==25&&start[1]==-42&&start[2]==13);
        }
    }
    float start[]{1,2,3},end[]{4,5,6};const float opposite[]{-1,0,0},zero[]{0,0,0};
    assert(!weapontrace::Align(start,end,from,opposite));
    assert(!weapontrace::Align(start,end,from,zero));
    const float huge[]{std::numeric_limits<float>::max(),0,0};
    assert(!weapontrace::Align(start,end,from,huge));
    const float nan[]{std::numeric_limits<float>::quiet_NaN(),0,0};
    assert(!weapontrace::Align(start,end,from,nan));
    assert(end[0]==4&&end[1]==5&&end[2]==6);
    assert(!weapontrace::Align(nullptr,end,from,from));
    // Native centre is not necessarily the cached HMD axis (native aim/recoil).
    // Rotate the native pre-spread centre to the barrel, keeping the cone.
    const float bias=.8f*pi/180.f;
    const float nativeCentre[]{std::cos(bias),0,std::sin(bias)};
    float biasedStart[]{0,0,0},biasedEnd[]{1000*nativeCentre[0],0,1000*nativeCentre[2]};
    assert(weapontrace::Align(biasedStart,biasedEnd,nativeCentre,from));
    assert(std::fabs(biasedEnd[2])<.001f&&std::fabs(biasedEnd[0]-1000)<.001f);
    // Current measured draw pose follows the weapon, not a cached controller
    // mount. Identity and age must refuse stale poses on switch/transition.
    scopepip::Frame pose{true,1,2,100,{.1f,1.5f,-.3f},{0,0,-1},{0,1,0},.016f,0};
    assert(pose.Current(1,2,250));assert(!pose.Current(1,3,100));
    assert(!pose.Current(1,2,251));assert(!pose.Current(1,2,99));
    assert(!scopepip::Frame{}.Current(1,2,100));
    weaponmuzzle::Witness witness{true,weaponreload::Kind::Valkyrie,1,2,100,{1,2,3},{1,0,0}};
    hipknife::V direction{99,99,99};assert(witness.Direction(1,2,250,direction)&&direction.x==1);
    assert(!witness.Direction(1,2,251,direction));assert(!witness.Direction(1,3,100,direction));
    witness.forward={0,0,0};assert(!witness.Direction(1,2,100,direction));
    witness.forward={nan[0],0,0};assert(!witness.Direction(1,2,100,direction));
    assert(direction.x==1&&direction.y==0&&direction.z==0);
}
