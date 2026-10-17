#include "../weapon_reload_runtime.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <vector>
#include <string>
using namespace weaponreload;
int main(int argc,char** argv){
    assert(argc==2);
    for(const auto& profile:weaponreloadproto::profiles){
        const std::string stem=Stem(profile.kind);
        std::ifstream in(std::string(argv[1])+"/"+stem+"/"+stem+"_reload.svwa",std::ios::binary|std::ios::ate);
        assert(in.good());const auto n=in.tellg();assert(n>0&&n<256*1024);
        std::vector<unsigned char> bytes((std::size_t)n);in.seekg(0);in.read((char*)bytes.data(),n);assert(in.good());
        weaponreloadanim::Data data;assert(weaponreloadanim::Decode(bytes.data(),bytes.size(),profile.kind,data));
        Animation animation;assert(animation.Measure(data));
        for(unsigned ms=0;ms<=5000;++ms){
            const auto phase=ms<100?Phase::WaitingNative:ms<600?Phase::Cooldown:Phase::Idle;
            const float frame=animation.RefillFrame(data,phase,ms<100?1000:1100,1000,1000+ms);
            assert(std::isfinite(frame)&&frame>=0&&frame<data.frames);
            if(ms>=Animation::RefillSeatMs)assert(frame==0);
            armbind::Mat sample[8];assert(weaponreloadanim::Sample(data,frame,sample));
        }
        assert(animation.RefillFrame(data,Phase::Carrying,1000,0,1100)==animation.outFrame);
        assert(animation.RefillFrame(data,Phase::NeedAmmo,1000,0,1100)==animation.Frame(data,Phase::NeedAmmo,1000,1100));
        weaponreloadproto::State state;weaponreloadproto::Frame frame;weaponreloadproto::Calibration cfg;
        cfg.ready=true;cfg.revision=1;frame.enabled=frame.liveIdentityVerified=true;
        frame.kind=profile.kind;frame.objectName=profile.objectName;frame.meshWedges=profile.meshWedges;
        auto& f=frame.gesture;f.context=f.supported=f.tracked=f.ammoFresh=true;
        f.pawn=20;f.weapon=30;f.capacity=30;f.clip=0;f.reserve=73;f.nowMs=1000;
        f.socket={.2f,1,-.4f};f.hand=f.socket;f.pouch={-.22f,1,.18f};frame.bodyYaw=.8f;
        assert(weaponreloadproto::Update(state,frame,cfg).event==weaponreloadproto::Event::None);
        frame.magazineReleaseDown=true;++f.nowMs;
        assert(weaponreloadproto::Update(state,frame,cfg).event==weaponreloadproto::Event::Ejected);
        frame.magazineReleaseDown=false;frame.bodyYaw=-2.f;++f.nowMs;
        weaponreloadproto::Update(state,frame,cfg);
        assert(state.transaction.bodyYaw==.8f); // later head/stick yaw cannot swing the pouch
    }
    unsigned poses=0;
    for(int y=-18;y<=18;++y)for(float ipd:{.045f,.064f,.085f})for(float h:{.35f,.8f,1.7f}){
        const float yaw=y*.17f;const hipknife::Q q{0,std::sin(yaw*.5f),0,std::cos(yaw*.5f)};
        const hipknife::V head{.3f,h,-.4f};hipknife::V measured;
        assert(weaponreloadproto::MidEye(head+hipknife::Rotate(q,{-ipd*.5f,0,0}),
            head+hipknife::Rotate(q,{ipd*.5f,0,0}),measured));
        const auto supply=weaponreloadproto::Supply(measured,yaw,{});
        const auto local=hipknife::Rotate({0,-q.y,0,q.w},supply.p-head);
        assert(std::fabs(local.x+.22f)<1e-5f&&std::fabs(local.y+.12f)<1e-5f&&std::fabs(local.z-.18f)<1e-5f);
        ++poses;
    }
    hipknife::V centre;assert(!weaponreloadproto::MidEye({},{},centre));
    assert(!weaponreloadproto::MidEye({},{1,0,0},centre));
    std::printf("PASS 3 original clip return tracks: 15003 samples, seating rest at180ms; %u mid-eye physical-left shoulder poses incl floor-seated\n",poses);
}
