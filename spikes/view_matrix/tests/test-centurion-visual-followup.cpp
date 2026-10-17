#define NOMINMAX
#include "../centurion_visual_followup.h"
#include "../centurion_tracked_cylinder.h"
#include "../centurion_clip_preview.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace centurionvisual;
static Mat PoseMatrix(Pose p){auto m=Identity();for(unsigned r=0;r<3;++r){
    V v{};if(r==0)v.x=1;if(r==1)v.y=1;if(r==2)v.z=1;v=hipknife::Rotate(p.q,v);
    m.m[r][0]=v.x;m.m[r][1]=v.y;m.m[r][2]=v.z;}
    m.m[3][0]=p.p.x;m.m[3][1]=p.p.y;m.m[3][2]=p.p.z;return m;}
static void Near(const Mat& a,const Mat& b,float e=.003f){for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)
    assert(std::fabs(a.m[r][c]-b.m[r][c])<e);}
int main(int argc,char** argv){
    using P=centurionreload::Phase;
    assert(Time(P::Loaded,100,0,100).frame==26&&Time(P::Loaded,100,0,10000).frame==26);
    for(unsigned ms=0;ms<16000;++ms){
        assert(Time(P::NeedAmmo,0,0,ms).frame==12);
        assert(Time(P::Open,0,0,ms).frame<=12);
        assert(Time(P::Carrying,0,0,ms).hideClip);
        assert(Time(P::Cooldown,100,100,100+ms).active==(ms<2600));
        for(const auto phase:{P::WaitingNative,P::Cooldown,P::Idle})
            assert(Time(phase,100,100,100+ms,true).active==(ms<280));
        // Flag ON must not change any pre-close open/carry/loaded presentation.
        for(const auto phase:{P::Open,P::NeedAmmo,P::Carrying,P::Loaded}){
            const auto old=Time(phase,0,0,ms),current=Time(phase,0,0,ms,true);
            assert(old.active==current.active&&old.hideClip==current.hideClip&&old.frame==current.frame);
        }
    }
    assert(!ReleaseNative(false,100)&&!ReleaseNative(true,0)&&ReleaseNative(true,100));
    assert(Time(P::Cooldown,100,100,379,true).active&&!Time(P::Cooldown,100,100,380,true).active);
    assert(Time(P::WaitingNative,100,100,100).frame==26);
    assert(Time(P::Cooldown,100,100,500).frame==32&&!Tail(100,99));
    centurionanim::Data d;d.ready=true;d.parent=d.inverse[0]=d.inverse[1]=Identity();
    for(auto& f:d.keys)for(auto& k:f)k[6]=1;
    if(argc==2){FILE* f=nullptr;assert(!fopen_s(&f,argv[1],"rb")&&f);
        std::vector<unsigned char> bytes(3208);assert(fread(bytes.data(),1,bytes.size(),f)==bytes.size());fclose(f);
        assert(centurionanim::Decode(bytes.data(),bytes.size(),d));
        float compact=0,old=0;const auto* idle=d.keys[0][0];
        for(unsigned frame=0;frame<53;++frame){float n=0;for(unsigned r=0;r<3;++r){
            const float v=d.keys[frame][0][r]-idle[r];n+=v*v;}
            if(frame<=12)compact=std::max(compact,std::sqrt(n));old=std::max(old,std::sqrt(n));}
        assert(compact<3.85f&&old>42.f);
        printf("private original travel measured: compact %.4f UU vs old maximum %.4f UU\n",compact,old);
    }
    unsigned cases=0;
    for(unsigned yaw=0;yaw<36;++yaw)for(float units:{26.25f,52.5f,105.f})for(float scale:{.5f,1.f,2.f}){
        const float a=float(yaw)*.174532925f;
        Mat world;assert(TrackingMap({.2f,1.2f,-.4f},a,-a*.7f,units,world));
        const Pose aim{{.2f,.8f,-.3f},{0,std::sin(a/2),0,std::cos(a/2)}};
        const Pose target{{-.22f,1.4f,.18f},{std::sin(a/2),0,0,std::cos(a/2)}};
        auto local=PoseMatrix({{2,4,6},{}}),gun=PoseMatrix(aim);
        for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)gun.m[r][c]*=scale;
        const auto root=PoseMatrix({{.1f,.2f,-.3f},{}});
        float spare[15][4];assert(Spare(d,root,local,gun,world,aim,target,spare));
        Mat skin,bind,invW;assert(armbind::FromGpu(reinterpret_cast<float(*)[4]>(spare),skin));
        assert(skeletal::Inverse(bind.m,d.inverse[0].m)&&skeletal::Inverse(invW.m,world.m));
        const auto actual=Product(Product(Product(bind,skin),Product(local,gun)),invW);
        assert(hipknife::Length(Point({},actual)-target.p)<.001f);
        for(unsigned slot=1;slot<5;++slot)for(unsigned r=0;r<3;++r){
            for(unsigned c=0;c<3;++c)assert(spare[slot*3+r][c]==0);
            assert(spare[slot*3+r][3]==spare[r][3]);}
        // Returning the target to the observed native clip pose roundtrips the
        // complete matrix, not only its position, at all scales/yaws.
        const auto expected=Product(Product(Product(bind,root),Product(local,gun)),invW);
        const Pose seated{Point({},expected),aim.q};
        assert(Spare(d,root,local,gun,world,aim,seated,spare));
        assert(armbind::FromGpu(reinterpret_cast<float(*)[4]>(spare),skin));Near(skin,root);
        ++cases;
    }
    float native[15][4]{},attached[15][4];for(unsigned s=0;s<5;++s)
        armbind::ToGpu(Identity(),reinterpret_cast<float(*)[4]>(native+s*3));
    RootHold hold;Mat held;assert(hold.Get(1,2,100,native,held));
    native[3][3]=70;assert(hold.Get(1,2,100,native,held));assert(held.m[3][0]==0);
    assert(Attached(d,{true,false,26},native,held,attached));assert(attached[3][3]==0);
    assert(hold.Get(1,2,101,native,held)&&held.m[3][0]==70);
    for(unsigned fail=0;fail<3;++fail){unsigned calls=0,draws=0;bool restored=false;
        const bool ok=Replay([&](const float (&)[15][4]){return ++calls!=fail;},[&](){++draws;return fail!=0;},
            native,attached,restored);
        assert(calls==2&&restored==(fail!=2)&&!ok);assert(draws==(fail==1?0u:1u));}
    // A live native root must keep the FIRST and second shot's rotation and
    // translation even during the short cylinder-close interpolation. Legacy
    // RootHold deliberately discards this motion: preserve that negative control.
    auto scaled=Identity();for(unsigned r=0;r<3;++r)scaled.m[r][r]=.02f;
    for(unsigned shot=0;shot<3;++shot){
        const auto recoil=PoseMatrix({{float(shot)*.7f,0,float(shot)*-.9f},
            centurionclip::Degrees(float(shot)*7,0,0)});
        for(unsigned slot=0;slot<5;++slot)
            armbind::ToGpu(recoil,reinterpret_cast<float(*)[4]>(native+slot*3));
        Mat live;assert(LiveRoot(native,live));Near(live,recoil);
        V socket;
        assert(centuriontracked::Attached(d,Time(P::Cooldown,100,100,100+shot*100,true),
            1.f-float(shot)/3.f,native,live,Identity(),scaled,Identity(),{},attached,socket));
        for(unsigned slot=1;slot<4;++slot){Mat body;
            assert(armbind::FromGpu(reinterpret_cast<float(*)[4]>(attached+slot*3),body));Near(body,recoil);}
    }
    Mat sentinel=Identity(),badLive;std::memcpy(badLive.m,sentinel.m,sizeof(badLive.m));
    native[3][0]=NAN;assert(!LiveRoot(native,badLive));Near(badLive,sentinel);
    assert(cases==324);
    Mat rejected;assert(!TrackingMap({},0,0,0,rejected));
    float unchangedPalette[15][4];std::memset(unchangedPalette,0x11,sizeof(unchangedPalette));float unchanged[15][4];std::memcpy(unchanged,unchangedPalette,sizeof(unchangedPalette));
    auto bad=Identity();bad.m[0][0]=NAN;
    assert(!Spare(d,bad,Identity(),Identity(),Identity(),{}, {},unchangedPalette));
    assert(!std::memcmp(unchangedPalette,unchanged,sizeof(unchangedPalette)));
    puts("PASS compact/open-until-flick, legacy tail unchanged, refill-only 280ms/live-root recoil, 324 full native/tracking scale roundtrips, replay faults; NOT Steam/PSVR2");
}
