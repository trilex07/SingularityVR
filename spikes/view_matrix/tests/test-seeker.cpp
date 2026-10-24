#include "../seeker_slot_policy.h"
#include "../seeker_projectile_policy.h"
#include <cassert>
#include <cstdio>

using armbind::Mat;
static Mat Identity(){Mat m;for(unsigned i=0;i<4;++i)m.m[i][i]=1;return m;}
static void Close(const Mat& a,const Mat& b,float eps=.003f){
    for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j)assert(std::fabs(a.m[i][j]-b.m[i][j])<eps);
}
int main(int argc,char** argv){
    if(argc==2){
        // Machine-readable compiled constants for the private asset oracle.
        for(const auto* m:{&seekergeometry::RootBind,&seekergeometry::LeftGrip})
            for(const auto& row:m->m)for(float x:row)std::printf("%.9g ",double(x));
        const auto p=seekergeometry::Muzzle,a=seekergeometry::Forward;
        std::printf("%.9g %.9g %.9g %.9g %.9g %.9g\n",double(p.x),double(p.y),double(p.z),double(a.x),double(a.y),double(a.z));
        return 0;
    }
    using namespace seekerslot;
    State state;Snapshot s;s.valid=true;s.pawn=1;s.current=10;s.count=2;
    s.items[0]={10,1,0,false};s.items[1]={20,2,0,true};
    state.Reconcile(s);assert(state.ToggleTarget(s)==20);
    s.current=20;state.Reconcile(s);assert(state.ToggleTarget(s)==10);
    s.current=10;state.Reconcile(s);assert(state.seeker==20); // native object retained
    s.count=1;state.Reconcile(s);assert(!state.seeker&&!state.ToggleTarget(s));
    s.count=2;s.pawn=2;state.Reconcile(s);assert(state.pawn==2&&state.ordinary==10);
    s.valid=false;state.Reconcile(s);assert(!state.pawn&&!state.seeker);
    s.valid=true;s.count=3;s.items[2]={30,2,0,true};state.Reconcile(s);assert(!state.seeker);
    for(unsigned caller=0;caller<500;++caller){
        assert(RetainQuery(true,true,true,true,1000+caller,1000)==(caller==0xd2||caller==0xe9));
        assert(!RetainQuery(false,true,true,true,1000+caller,1000));
        assert(!RetainQuery(true,false,true,true,1000+caller,1000));
        assert(!RetainQuery(true,true,false,true,1000+caller,1000));
        assert(!RetainQuery(true,true,true,false,1000+caller,1000));
    }
    for(float yaw:{0.f,1.57f,-1.57f,3.14f}){
        const hipknife::V head{1,1.2f,-2};
        const auto at=bodyequipment::Layout{}.At(bodyequipment::Slot::Heavy,head,yaw).centre;
        assert(BackGrip(true,true,true,false,true,at,head,yaw));
        assert(!BackGrip(false,true,true,false,true,at,head,yaw));
        assert(!BackGrip(true,true,true,true,true,at,head,yaw));
        assert(!BackGrip(true,true,false,false,true,at,head,yaw));
        assert(!BackGrip(true,true,true,false,true,at+hipknife::V{0,.5f,0},head,yaw));
    }
    assert(seekergeometry::Body(0,0,12444,0,14990));
    assert(seekergeometry::Body(0,0,12436,0,14990));
    assert(!seekergeometry::Body(0,12436,8,44970,6));
    assert(!seekergeometry::Body(0,0,11387,0,12834));
    seekergeometry::Witness w;
    Mat skin=Identity(),local=Identity();local.m[3][1]=-100;
    assert(seekergeometry::Capture(skin,local,1,20,1000,w));
    assert(w.Current(1,20,1150));assert(!w.Current(1,20,1151));assert(!w.Current(1,21,1000));
    unsigned cases=0;
    for(float angle:{0.f,.7f,1.57f,-1.57f,3.14f})for(float scale:{.8f,1.f,1.2f}){
        Mat gun=Identity();gun.m[0][0]=gun.m[1][1]=std::cos(angle)*scale;
        gun.m[0][1]=std::sin(angle)*scale;gun.m[1][0]=-std::sin(angle)*scale;gun.m[2][2]=scale;
        gun.m[3][0]=15;gun.m[3][2]=-8;
        Mat wristBind=Identity();wristBind.m[3][0]=49;wristBind.m[3][1]=90;
        Mat animated=Identity();animated.m[3][2]=5;float gpu[3][4];armbind::ToGpu(animated,gpu);
        Mat target;assert(seekergeometry::SupportTarget(w,1,20,1100,wristBind,gpu,local,gun,target));
        const auto desired=seekergeometry::Product(seekergeometry::Product(seekergeometry::LeftGrip,w.rootLocal),gun);
        const auto before=seekergeometry::Product(seekergeometry::Product(wristBind,animated),local);
        Close(seekergeometry::Product(before,target),desired);
        // Exercise the exact shader palette + LocalDeltaPair + eye submission order.
        float delta[4][4],right[4][4],changed[3][4];
        assert(skeletal::LocalDeltaPair(delta,right,local.m,gun.m,target.m,gun.m));
        skeletal::TransformBone(changed,gpu,delta);Mat transformed;assert(armbind::FromGpu(changed,transformed));
        const auto final=seekergeometry::Product(seekergeometry::Product(seekergeometry::Product(wristBind,transformed),local),gun);
        Close(final,desired);
        for(float eye:{-1.6f,1.6f}){auto e=Identity();e.m[3][0]=eye;
            Close(seekergeometry::Product(final,e),seekergeometry::Product(desired,e));}
        seekergeometry::SupportFrame frame{true,1,20,10,local,target};Mat out;
        assert(frame.Read(1,20,10,local,out));assert(!frame.Read(1,20,11,local,out));
        assert(!frame.Read(1,21,10,local,out));auto bad=local;bad.m[3][0]+=1;assert(!frame.Read(1,20,10,bad,out));
        seekerprojectile::Launch launch;
        assert(seekerprojectile::Build(w,1,20,1100,gun,{1000,2000,3000},launch));
        int rot[3]{0,0,777};assert(seekerprojectile::Spread(launch,20,0,0,rot));assert(rot[2]==777);
        const double k=6.283185307179586/65536.;
        hipknife::V direction{float(std::cos(rot[0]*k)*std::cos(rot[1]*k)),float(std::cos(rot[0]*k)*std::sin(rot[1]*k)),float(std::sin(rot[0]*k))};
        assert(hipknife::Dot(direction,launch.axis)>.99999f);
        int cone[3]{100,-240,987};assert(seekerprojectile::Spread(launch,20,0,0,cone));
        hipknife::V spread{float(std::cos(cone[0]*k)*std::cos(cone[1]*k)),float(std::cos(cone[0]*k)*std::sin(cone[1]*k)),float(std::sin(cone[0]*k))};
        assert(std::fabs(hipknife::Dot(spread,launch.axis)-std::cos(100*k)*std::cos(-240*k))<.00001);
        assert(cone[2]==987);assert(!seekerprojectile::Spread(launch,21,0,0,cone));
        ++cases;
    }
    std::printf("PASS Seeker slot lifecycle/exact caller gates/rotated back Grip; %u skin/support/stereo/launch-cone cases\n",cases);
}
