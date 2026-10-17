#define NOMINMAX
#include "../centurion_tracked_cylinder.h"
#include "../centurion_clip_preview.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace centuriontracked;
static Mat M(Pose p){Mat m;assert(PoseMatrix(p,m));return m;}
static void NearMatrix(const Mat& a,const Mat& b){for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)
    assert(std::fabs(a.m[r][c]-b.m[r][c])<.004f);}
int main(int argc,char** argv){
    centurionanim::Data d;d.ready=true;d.parent=d.inverse[0]=d.inverse[1]=Identity();
    for(auto& f:d.keys)for(auto& k:f)k[6]=1;
    if(argc==2){FILE* file=nullptr;assert(!fopen_s(&file,argv[1],"rb")&&file);
        std::vector<unsigned char> data(3208);assert(fread(data.data(),1,data.size(),file)==data.size());fclose(file);
        assert(centurionanim::Decode(data.data(),data.size(),d));}
    unsigned cases=0;
    for(int yaw=-180;yaw<180;yaw+=10)for(int pitch:{-70,0,70})for(int roll:{-80,0,80})
    for(float units:{26.25f,52.5f,105.f})for(float scale:{.5f,1.f,2.f}){
        float original[15][4];const auto initial=M({{2,3,4},centurionclip::Degrees(10,20,-15)});
        for(unsigned slot=0;slot<5;++slot)armbind::ToGpu(initial,reinterpret_cast<float(*)[4]>(original+slot*3));
        const Pose start{{.2f,.6f,-.4f},centurionclip::Degrees(float(pitch),float(yaw),float(roll))};
        const auto local=M({{20,-40,6},centurionclip::Degrees(-7,11,14)});
        auto gun=M({{10,8,-4},centurionclip::Degrees(12,23,-9)});
        for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)gun.m[r][c]*=scale;
        Mat tracking;assert(TrackingMap({.3f,.4f,-.6f},.7f,-1.1f,units,tracking));
        RootHold hold;Mat held;assert(hold.Get(1,2,3,original,local,gun,tracking,start,held));NearMatrix(held,initial);
        const auto movingLocal=M({{-100,80,-44},centurionclip::Degrees(31,-42,19)});
        auto movingGun=M({{12,-28,40},centurionclip::Degrees(-15,140,50)});
        for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)movingGun.m[r][c]*=scale;
        const Pose moved{start.p+V{-.3f,.15f,.2f},hipknife::Multiply(start.q,centurionclip::Degrees(30,45,20))};
        float native[15][4];const auto animated=M({{125,-90,60},centurionclip::Degrees(-70,50,90)});
        for(unsigned slot=0;slot<5;++slot)armbind::ToGpu(animated,reinterpret_cast<float(*)[4]>(native+slot*3));
        assert(hold.Get(1,2,3,native,movingLocal,movingGun,tracking,moved,held));
        Mat invTracking,invAim;assert(Inverse(tracking,invTracking)&&Inverse(M(moved),invAim));
        const auto chain=Product(movingLocal,movingGun);
        NearMatrix(Product(Product(Product(held,chain),invTracking),invAim),hold.controllerLocal);
        // The old model-only hold is a falsifier: it does NOT survive this native chain change.
        assert(hipknife::Length(Point({},Product(Product(initial,chain),invTracking))-
            Point({},Product(Product(held,chain),invTracking)))>.1f);
        float closed[15][4],opened[15][4],hidden[15][4];V c,o,h;
        assert(Attached(d,{true,false,12},0,native,held,movingLocal,movingGun,tracking,moved,closed,c));
        assert(Attached(d,{true,false,12},1,native,held,movingLocal,movingGun,tracking,moved,opened,o));
        assert(Attached(d,{true,true,12},1,native,held,movingLocal,movingGun,tracking,moved,hidden,h));
        const auto left=hipknife::Rotate(moved.q,{-1,0,0});const auto delta=o-c;
        assert(hipknife::Dot(delta,left)>.01f&&hipknife::Length(delta-left*hipknife::Dot(delta,left))<.0001f);
        Mat bind,skin;assert(Inverse(d.inverse[0],bind));
        assert(armbind::FromGpu(reinterpret_cast<float(*)[4]>(opened),skin));
        const auto actual=Product(Product(Product(bind,skin),chain),invTracking);
        assert(hipknife::Length(Point({},actual)-o)<.001f&&hipknife::Length(h-o)<1e-6f);
        for(unsigned r=0;r<3;++r)for(unsigned col=0;col<3;++col)assert(hidden[r][col]==0);
        for(unsigned slot=1;slot<4;++slot){Mat bone;assert(armbind::FromGpu(reinterpret_cast<float(*)[4]>(opened+slot*3),bone));NearMatrix(bone,held);}
        const auto prior=hold.controllerLocal;auto invalid=movingLocal;invalid.m[0][0]=NAN;
        assert(!hold.Get(1,2,99,native,invalid,movingGun,tracking,moved,held));NearMatrix(prior,hold.controllerLocal);
        ++cases;
    }
    SocketWitness witness{1,2,3,100,{.1f,.2f,.3f},true};
    assert(witness.Current(1,2,3,100)&&witness.Current(1,2,3,200));
    assert(!witness.Current(1,2,3,201)&&!witness.Current(1,2,3,99)&&!witness.Current(1,2,4,100)&&
        !witness.Current(1,9,3,100)&&!witness.Current(9,2,3,100));
    using P=centurionreload::Phase;using E=centurionreload::Event;
    centurionreload::Config cfg;cfg.cylinderGestures=cfg.circleOpens=true;
    centurionreload::Frame f;f.enabled=f.context=f.tracked=f.supported=f.ammoFresh=true;
    f.pawn=1;f.weapon=2;f.clip=0;f.reserve=18;f.capacity=6;f.nowMs=1000;
    f.pouch=centurionreload::Pouch({0,.5f,0},0).p;f.gun={{.2f,.3f,-.5f},{}};f.socket=f.gun.p;
    centurionreload::State s;auto tick=[&](unsigned dt=20){f.nowMs+=dt;return centurionreload::Update(s,f,cfg);};
    tick();f.releaseCylinder=true;assert(tick().event==E::Opened&&s.phase==P::Open);f.releaseCylinder=false;
    // Circle alone opens, no side flick or displacement. Tilt only dumps cases once.
    for(unsigned i=0;i<10;++i)assert(tick().event==E::None);
    f.gun.q=centurionclip::Degrees(-45,0,0);assert(tick().event==E::None);
    assert(tick(100).event==E::Dumped&&s.phase==P::NeedAmmo);assert(tick().event==E::None);
    f.hand=f.pouch;f.grip=true;assert(tick().event==E::GrabFresh);
    // A stale witness cannot authorize a refill, even at zero distance.
    f.hand=f.socket;f.socketFresh=false;f.grip=false;assert(tick().event==E::Discarded);
    f.socketFresh=true;f.hand=f.pouch;f.grip=true;assert(tick().event==E::GrabFresh);
    f.insertOffset={0,0,-.08f};f.hand=f.socket-V{0,0,-.08f};f.grip=false;
    assert(tick().event==E::Loaded);
    f.gun.q={};tick(120);f.gun.p.x+=.09f;assert(tick().requestNative&&s.phase==P::WaitingNative);
    assert(!tick().requestNative);f.clip=6;f.reserve=12;assert(tick().event==E::Acknowledged);
    // Transactional failure, fresh transaction capture and context cancellation.
    float original[15][4];for(unsigned slot=0;slot<5;++slot)
        armbind::ToGpu(Identity(),reinterpret_cast<float(*)[4]>(original+slot*3));
    RootHold rearm;Mat result,tracking;assert(TrackingMap({},0,0,52.5f,tracking));
    assert(rearm.Get(1,2,1,original,Identity(),Identity(),tracking,{},result));
    original[3][3]=12;assert(rearm.Get(1,2,2,original,Identity(),Identity(),tracking,{},result));
    assert(std::fabs(result.m[3][0]-12)<.001f);
    float unchanged[15][4],badOutput[15][4];std::memset(unchanged,0x11,sizeof(unchanged));
    std::memcpy(badOutput,unchanged,sizeof(unchanged));V sentinel{1,2,3};
    auto bad=Identity();bad.m[0][0]=NAN;
    assert(!Attached(d,{true,false,12},1,original,result,bad,Identity(),tracking,{},badOutput,sentinel));
    assert(!std::memcmp(unchanged,badOutput,sizeof(unchanged))&&sentinel.x==1&&sentinel.y==2&&sentinel.z==3);
    s={};f.clip=0;f.reserve=18;f.context=false;f.releaseCylinder=true;
    assert(!tick().requestNative&&s.phase==P::Idle);
    assert(OpenAmount(P::Open,100,0,100)==0&&OpenAmount(P::Open,100,0,260)==1);
    assert(OpenAmount(P::Loaded,100,0,9000)==1&&OpenAmount(P::WaitingNative,100,100,380)==0);
    printf("PASS tracked cylinder: %u full-chain/left-side/socket cases incl tilt/native component motion; Circle opens/dump/shoulder/insert/flick, stale reject; NOT Steam/PSVR2\n",cases);
}
