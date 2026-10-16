#define NOMINMAX
#include "../weapon_reload_prototypes.h"
#include "../knife_renderer.h"
#include <cassert>
#include <cstdio>
#include <limits>
#include <vector>
using namespace weaponreloadproto;
using E=weaponreloadproto::Event;
using V=hipknife::V;
struct Fixture {
    State s;Frame f;Calibration c;
    const V withdrawn{.2f,1,-.8f};
    explicit Fixture(const Profile& p){
        f.enabled=true;f.liveIdentityVerified=true;f.kind=p.kind;
        f.objectName=p.objectName;f.meshWedges=p.meshWedges;
        auto& in=f.gesture;
        in.context=in.tracked=in.supported=in.ammoFresh=true;
        in.pawn=20;in.weapon=10;in.clip=2;in.reserve=25;in.capacity=6;
        in.socket={.2f,1,-.5f};in.pouch=Supply({0,1.1f,0},0,c.shoulder).p;in.hand=in.socket;
        c.ready=true;c.revision=1;c.gesture.ackMs=8000;
    }
    Result Tick(bool grip,V position,bool circle=false){
        f.gesture.nowMs+=20;f.gesture.grip=grip;f.gesture.hand=position;
        f.magazineReleaseDown=circle;
        return Update(s,f,c);
    }
    void Start(){
        Tick(false,f.gesture.socket);
        const auto r=Tick(false,f.gesture.socket,true);
        assert(r.event==E::Ejected&&r.ejectMagazine&&!r.reservePhysicalLeft&&!r.reloadIntent);
    }
    void Insert(){
        Start(); // no old-magazine grip, pull, drop, tilt or gun flick
        assert(Tick(true,f.gesture.pouch).event==E::GrabFresh);
        assert(Tick(true,f.gesture.socket).reservePhysicalLeft);
        assert(Tick(false,f.gesture.socket).reloadIntent);
    }
};
static void TestDefaultsAndIdentity(const Profile& p){
    assert(!Frame{}.enabled&&!Calibration{}.ready&&!Find(Kind::None));
    for(int gate=0;gate<19;++gate){
        Fixture x(p);
        if(gate==0)x.f.enabled=false;
        if(gate==1)x.f.liveIdentityVerified=false;
        if(gate==2)x.f.kind=Kind::None;
        if(gate==3)x.f.objectName=nullptr;
        if(gate==4)x.f.objectName="RvWeaponShared"; // not an object identity
        if(gate==5)x.f.meshWedges=5799; // Centurion cannot enter these profiles
        if(gate==6)x.f.aimHand=0;
        if(gate==7)x.c.ready=false;
        if(gate==8)x.c.revision=0;
        if(gate==9)x.f.gesture.pawn=0;
        if(gate==10)x.f.gesture.weapon=0;
        if(gate==11)x.f.gesture.socket.x=std::numeric_limits<float>::quiet_NaN();
        if(gate==12)x.f.gesture.firing=true;
        if(gate==13)x.f.gesture.ammoFresh=false;
        if(gate==14)x.f.gesture.tracked=false;
        if(gate==15)x.f.gesture.context=false;
        if(gate==16)x.f.gesture.clip=x.f.gesture.capacity;
        if(gate==17)x.f.gesture.reserve=0;
        if(gate==18)x.f.gesture.supported=false;
        x.Tick(false,x.f.gesture.socket);
        auto r=x.Tick(false,x.f.gesture.socket,true);
        assert(r.event==E::None&&!r.reloadIntent&&!r.reservePhysicalLeft&&!r.ejectMagazine);
    }
    for(auto other:profiles)if(other.kind!=p.kind){
        Fixture x(p);x.f.meshWedges=other.meshWedges;
        x.Tick(false,x.f.gesture.socket);
        assert(x.Tick(false,x.f.gesture.socket,true).event==E::None);
    }
}
static void TestCancellation(const Profile& p){
    for(int fault=0;fault<16;++fault){
        Fixture x(p);x.Start();assert(x.Tick(true,x.f.gesture.pouch).event==E::GrabFresh);
        const Frame saved=x.f;const Calibration savedCal=x.c;
        if(fault==0){const auto& next=profiles[p.kind==Kind::Kasimov?0:2];
            x.f.kind=next.kind;x.f.objectName=next.objectName;x.f.meshWedges=next.meshWedges;}
        if(fault==1)++x.c.revision;
        if(fault==2)x.c.gesture.socketLocal.x+=.01f; // same revision still cancels
        if(fault==3)x.c.ready=false;
        if(fault==4)x.f.enabled=false;
        if(fault==5)x.f.aimHand=0;
        if(fault==6)x.f.gesture.context=false;
        if(fault==7)x.f.gesture.ammoFresh=false;
        if(fault==8)x.f.gesture.weapon++;
        if(fault==9)x.f.gesture.pawn++;
        if(fault==10)x.c.shoulder.left+=.01f;
        if(fault==11)x.f.gesture.firing=true;
        if(fault==12)x.f.gesture.capacity++;
        if(fault==13)x.f.gesture.nowMs=0;
        if(fault==14)x.f.gesture.socket.z=std::numeric_limits<float>::quiet_NaN();
        if(fault==15)x.f.gesture.handBusy=true;
        auto r=x.Tick(true,x.f.gesture.socket);
        assert(r.event==E::Cancelled&&r.reservePhysicalLeft&&!r.reloadIntent);
        const auto now=std::max(x.f.gesture.nowMs,saved.gesture.nowMs);x.f=saved;x.c=savedCal;x.f.gesture.nowMs=now;
        assert(!x.Tick(true,x.f.gesture.socket).reloadIntent);
        x.f.gesture.nowMs+=x.c.gesture.cooldownMs+20;
        assert(!x.Tick(false,x.f.gesture.socket).reservePhysicalLeft);
        assert(x.Tick(false,x.f.gesture.socket,true).event==E::Ejected);
    }
}
static void TestAmmoAndTimeout(const Profile& p){
    Fixture independent(p);independent.c.gesture.cylinderGestures=true;
    independent.c.gesture.insertRadius=9;independent.c.gesture.grabWindowMs=60000;
    independent.f.gesture.releaseCylinder=true;
    assert(independent.Tick(false,independent.f.gesture.socket).event==E::None);
    independent.Insert(); // Centurion cylinder/large socket/window never leak into magazines
    Fixture edge(p);edge.Tick(false,edge.withdrawn);edge.Tick(true,edge.withdrawn);
    assert(edge.Tick(true,edge.f.gesture.socket).event==E::None); // rising-edge only
    Fixture x(p);x.Insert();
    for(int i=0;i<50;++i)assert(!x.Tick(false,x.f.gesture.socket).reloadIntent);
    x.f.gesture.clip=6;x.f.gesture.reserve=25;
    assert(x.Tick(false,x.f.gesture.socket).event==E::None); // pickup is not reload
    x.f.gesture.reserve=21;
    assert(x.Tick(false,x.f.gesture.socket).event==E::Acknowledged);
    Fixture missing(p);missing.Insert();missing.f.gesture.nowMs+=8000;
    assert(missing.Tick(false,missing.f.gesture.socket).event==E::Rejected);
    Fixture changed(p);changed.Insert();changed.f.gesture.capacity=7;
    assert(changed.Tick(false,changed.f.gesture.socket).event==E::Cancelled);
    // No weapon-specific hardcoded capacity; producer owns these values.
    Fixture arbitrary(p);arbitrary.f.gesture.capacity=17;arbitrary.Insert();
    arbitrary.f.gesture.clip=17;arbitrary.f.gesture.reserve=10;
    assert(arbitrary.Tick(false,arbitrary.f.gesture.socket).event==E::Acknowledged);
    Fixture empty(p);empty.f.gesture.clip=0;empty.f.gesture.reserve=3;empty.Insert();
    empty.f.gesture.clip=3;empty.f.gesture.reserve=0;
    assert(empty.Tick(false,empty.f.gesture.socket).event==E::Acknowledged); // partial refill, never invent six rounds
    Fixture early(p);early.Start();early.f.gesture.clip=6;early.f.gesture.reserve=21;
    assert(early.Tick(false,early.f.gesture.socket).event==E::Cancelled); // conserved change before insert is NOT ours
}
static void TestEveryPhase(const Profile& p){
    for(auto phase:{Phase::NeedAmmo,Phase::Carrying,Phase::WaitingNative}){
        Fixture x(p);x.Tick(false,x.f.gesture.socket);
        auto& s=x.s.transaction;
        s.phase=phase;s.pawn=x.f.gesture.pawn;s.weapon=x.f.gesture.weapon;
        s.startClip=2;s.startReserve=25;s.startCapacity=6;s.since=x.f.gesture.nowMs;
        s.ownsGrip=true;s.priorGrip=true;
        // Changing calibration while waiting must not credit a coincidental
        // conserved ammo change to the old weapon/socket transaction.
        ++x.c.revision;
        x.f.gesture.clip=6;x.f.gesture.reserve=21;
        const auto result=x.Tick(true,x.f.gesture.socket);
        assert(result.event==E::Cancelled&&result.reservePhysicalLeft&&!result.reloadIntent);
    }
    Fixture drop(p);drop.Start();
    assert(drop.Tick(true,drop.f.gesture.pouch).event==E::GrabFresh);
    assert(drop.Tick(false,drop.withdrawn).event==E::Discarded);
    assert(drop.f.gesture.clip==2&&drop.f.gesture.reserve==25);
    assert(drop.Tick(true,drop.f.gesture.socket).event==E::None); // new pouch grip required
}
static void TestCircleAndOwnership(const Profile& p){
    Fixture pull(p);pull.Tick(false,pull.f.gesture.socket);
    for(int i=0;i<12;++i){
        assert(pull.Tick(true,pull.f.gesture.socket).event==E::None);
        assert(pull.Tick(true,pull.withdrawn).event==E::None);
        assert(pull.Tick(false,pull.withdrawn).event==E::None);
        assert(pull.Tick(true,pull.f.gesture.pouch).event==E::None);
        pull.Tick(false,pull.withdrawn);
    } // grip alone cannot eject or obtain fresh ammo
    Fixture held(p);
    for(int i=0;i<50;++i)assert(!held.Tick(false,held.f.gesture.socket,true).ejectMagazine);
    held.Tick(false,held.f.gesture.socket);
    assert(held.Tick(false,held.f.gesture.socket,true).ejectMagazine);
    for(int i=0;i<50;++i)assert(!held.Tick(false,held.f.gesture.socket,true).ejectMagazine);
    // A right-hand button does not take over the physical left hand. Support,
    // knife proximity and an already-held object only block the SHOULDER grab.
    for(int busy=0;busy<3;++busy){
        Fixture x(p);
        x.f.gesture.supportLatched=busy==0;x.f.gesture.handBusy=busy==1;x.f.gesture.knifeNear=busy==2;
        x.Tick(true,x.f.gesture.socket);const auto e=x.Tick(true,x.f.gesture.socket,true);
        assert(e.ejectMagazine&&!e.reservePhysicalLeft&&!e.reloadIntent);
        assert(x.Tick(true,x.f.gesture.pouch).event==E::None);
        x.Tick(false,x.f.gesture.pouch);
        assert(x.Tick(true,x.f.gesture.pouch).event==E::None); // busy hand never stolen
        x.f.gesture.supportLatched=x.f.gesture.handBusy=x.f.gesture.knifeNear=false;
        assert(x.Tick(true,x.f.gesture.pouch).event==E::None); // no fake grip edge
        x.Tick(false,x.f.gesture.pouch);
        assert(x.Tick(true,x.f.gesture.pouch).event==E::GrabFresh);
        assert(!x.Tick(true,x.f.gesture.socket,true).ejectMagazine); // Circle while carrying ignored
        assert(x.Tick(false,x.f.gesture.socket).reloadIntent); // NO closing gesture
    }
    for(int fault=0;fault<4;++fault){
        Fixture x(p);x.Start();
        if(fault==0)x.f.gesture.context=false;
        if(fault==1)x.f.gesture.tracked=false;
        if(fault==2)x.f.liveIdentityVerified=false;
        if(fault==3)x.f.enabled=false;
        assert(x.Tick(false,x.f.gesture.socket,true).event==E::Cancelled);
        x.f.gesture.context=x.f.gesture.tracked=x.f.liveIdentityVerified=x.f.enabled=true;
        for(int i=0;i<50;++i)assert(!x.Tick(false,x.f.gesture.socket,true).ejectMagazine);
        x.Tick(false,x.f.gesture.socket);
        assert(x.Tick(false,x.f.gesture.socket,true).ejectMagazine);
    }
    Fixture cycle(p);cycle.Start();
    assert(cycle.Tick(true,cycle.f.gesture.pouch,true).event==E::GrabFresh);
    assert(cycle.Tick(false,cycle.f.gesture.socket,true).reloadIntent);
    cycle.f.gesture.clip=6;cycle.f.gesture.reserve=21;
    assert(cycle.Tick(false,cycle.f.gesture.socket,true).event==E::Acknowledged);
    for(int i=0;i<40;++i)assert(!cycle.Tick(false,cycle.f.gesture.socket,true).ejectMagazine);
    cycle.f.gesture.clip=1;
    assert(!cycle.Tick(false,cycle.f.gesture.socket,true).ejectMagazine); // still same held press
    cycle.Tick(false,cycle.f.gesture.socket);
    assert(cycle.Tick(false,cycle.f.gesture.socket,true).ejectMagazine);
    Fixture steal(p);steal.Start();steal.Tick(true,steal.f.gesture.pouch);
    steal.f.gesture.supportLatched=true;
    const auto cancelled=steal.Tick(true,steal.f.gesture.socket);
    assert(cancelled.event==E::Cancelled&&cancelled.reservePhysicalLeft&&!cancelled.reloadIntent);
}
static void TestCalibration(){
    Calibration c;c.ready=true;c.revision=1;assert(Valid(c));
    for(int fault=0;fault<9;++fault){
        auto bad=c;
        if(fault==0)bad.gesture.socketLocal.x=std::numeric_limits<float>::quiet_NaN();
        if(fault==1)bad.gesture.socketRadius=bad.gesture.pullDistance;
        if(fault==2)bad.gesture.pouchRadius=0;
        if(fault==3)bad.shoulder.down=-1;
        if(fault==4)bad.shoulder.back=std::numeric_limits<float>::infinity();
        if(fault==5)bad.gesture.ackMs=0;
        if(fault==6)bad.gesture.timeoutMs=60001;
        if(fault==7)bad.gesture.cooldownMs=5001;
        if(fault==8)bad.gesture.pullDistance=std::numeric_limits<float>::quiet_NaN();
        assert(!Valid(bad));
    }
    for(int degrees=-180;degrees<=180;++degrees){
        const float yaw=float(degrees)*3.14159265359f/180.f;const V head{.3f,.4f,-.2f};
        const auto p=Supply(head,yaw,c.shoulder);
        const auto delta=hipknife::Rotate({0,-std::sin(yaw*.5f),0,std::cos(yaw*.5f)},p.p-head);
        assert(std::fabs(delta.x+.22f)<1e-6f&&std::fabs(delta.y+.12f)<1e-6f&&std::fabs(delta.z-.18f)<1e-6f);
    }
}
static void ValidateAsset(const char* path,unsigned expectedVerts,unsigned expectedTris){
    FILE* file=nullptr;assert(fopen_s(&file,path,"rb")==0&&file);
    assert(!std::fseek(file,0,SEEK_END));const long bytes=std::ftell(file);
    assert(bytes>0&&bytes<1024*1024);std::rewind(file);
    std::vector<unsigned char> raw((size_t)bytes);
    assert(std::fread(raw.data(),1,raw.size(),file)==raw.size());std::fclose(file);
    std::vector<hipknife::Vertex> vertices;std::vector<WORD> indices;
    assert(hipknife::Decode(raw.data(),raw.size(),vertices,indices));
    assert(vertices.size()==expectedVerts&&indices.size()==expectedTris*3);
}
int main(int argc,char** argv){
    assert(!std::strcmp(Name(E::Ejected),"magazine-ejected"));
    TestCalibration();
    unsigned transactions=0;
    for(const auto& p:profiles){
        TestDefaultsAndIdentity(p);TestCancellation(p);TestAmmoAndTimeout(p);TestEveryPhase(p);TestCircleAndOwnership(p);
        for(unsigned cycle=0;cycle<10000;++cycle){
            Fixture x(p);x.Insert();
            for(unsigned i=0;i<4;++i)assert(!x.Tick(false,x.f.gesture.socket).reloadIntent);
            x.f.gesture.clip=6;x.f.gesture.reserve=21;
            assert(x.Tick(false,x.f.gesture.socket).event==E::Acknowledged);++transactions;
        }
    }
    assert(transactions==30000);
    if(argc>1){assert(argc==4);ValidateAsset(argv[1],547,596);ValidateAsset(argv[2],242,198);ValidateAsset(argv[3],170,180);}
    std::printf("next-weapon reload L1 PASS: 3 profiles, Circle eject/shoulder/insert with NO flick, %u conserved synthetic transactions, default OFF/identity/calibration/ownership/held-input gates; native backend NOT wired; private assets %s\n",
        transactions,argc==4?"validated":"not requested");
}
