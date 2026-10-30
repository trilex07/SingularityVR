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
            // Release body/scope immediately on insertion, not after the short
            // child-only seating tail or a delayed HUD ACK. Flag OFF unchanged.
            for(const auto p:{Phase::Idle,Phase::NeedAmmo,Phase::Carrying,Phase::WaitingNative,Phase::Cooldown}){
                const bool oldHeld=Active(p)||1000+ms<5000;
                assert(Animation::HoldRoot(false,true,p,1000,5000,1000+ms)==oldHeld);
                assert(!Animation::HoldRoot(true,true,p,1000,5000,1000+ms));
                assert(!Animation::HoldRoot(false,false,p,1000,5000,1000+ms));
                assert(Animation::AnimateMagazine(true,p,1000,1000+ms,false)==(ms<Animation::RefillSeatMs));
                assert(Animation::AnimateMagazine(false,p,1000,1000+ms,oldHeld)==(Active(p)||oldHeld));
            }
        }
        assert(!Animation::AnimateMagazine(true,Phase::WaitingNative,1000,999,false));
        assert(Animation::HoldRoot(true,true,Phase::Carrying,0,0,1000));
        assert(Animation::AnimateMagazine(true,Phase::Carrying,0,1000,true));
        const unsigned regs=data.slots*3;
        for(unsigned shot=0;shot<3;++shot){
            float native[16*12]{},out[16*12]{},allTracks[16*12]{};
            for(unsigned slot=0;slot<data.slots;++slot){
                const float angle=float(shot)*.13f;armbind::Mat m{};
                m.m[0][0]=m.m[1][1]=std::cos(angle);m.m[0][1]=std::sin(angle);m.m[1][0]=-std::sin(angle);
                m.m[2][2]=m.m[3][3]=1;m.m[3][0]=float(slot)+float(shot)*.7f;m.m[3][2]=float(shot)*-.9f;
                armbind::ToGpu(m,reinterpret_cast<float(*)[4]>(native+slot*12));
            }
            const auto at=1000+shot*60;const float seatFrame=animation.RefillFrame(data,Phase::Cooldown,1000,1000,at);
            assert(weaponreloadanim::Palette(data,true,seatFrame,native,regs,allTracks));
            assert(SeatingPalette(data,seatFrame,native,regs,out));
            for(unsigned slot=0;slot<data.slots;++slot){
                const auto* expected=SeatingSlot(profile.kind,slot)?allTracks:native;
                assert(!std::memcmp(out+slot*12,expected+slot*12,12*sizeof(float)));
            } // every non-magazine native bone, including root/scope/fire tracks, byte-exact
            float parent[16*12]{};assert(ParentPalette(data,native,regs,nullptr,parent));
            assert(!std::memcmp(parent,native,regs*4*sizeof(float)));
            std::memset(out,0x11,sizeof(out));float sentinel[16*12];std::memcpy(sentinel,out,sizeof(out));
            assert(!SeatingPalette(data,seatFrame,native,regs-1,out)&&!std::memcmp(out,sentinel,sizeof(out)));
            native[data.root*12]=NAN;
            assert(!SeatingPalette(data,seatFrame,native,regs,out)&&!std::memcmp(out,sentinel,sizeof(out)));
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
    std::printf("PASS 4 original clip tracks: 180ms child-only seat/live first-shot bones, immediate root/scope release and legacy OFF; %u physical-left shoulder poses\n",poses);
}
