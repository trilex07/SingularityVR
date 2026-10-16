#include "../weapon_reload_runtime.h"
#include "../centurion_native_reload.h"
#include "../centurion_clip_preview.h"
#include <cassert>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>
using namespace weaponreload;
static armbind::Mat Identity(){armbind::Mat m;for(unsigned i=0;i<4;++i)m.m[i][i]=1;return m;}
static std::vector<unsigned char> Read(const std::string& file){
    FILE* fp=nullptr;assert(!fopen_s(&fp,file.c_str(),"rb")&&fp);
    assert(!fseek(fp,0,SEEK_END));const long n=ftell(fp);assert(n>0&&n<256*1024);rewind(fp);
    std::vector<unsigned char> bytes((size_t)n);assert(fread(bytes.data(),1,bytes.size(),fp)==bytes.size());fclose(fp);return bytes;
}
static void TestOriginal(const weaponreloadanim::Spec& spec,const std::string& root){
    const auto kind=spec.kind;const auto stem=std::string(Stem(kind));
    const auto bytes=Read(root+"/"+stem+"/"+stem+"_reload.svwa");
    auto data=std::make_unique<weaponreloadanim::Data>();assert(weaponreloadanim::Decode(bytes.data(),bytes.size(),kind,*data));
    assert(Hash(bytes.data(),(unsigned)bytes.size())==originalAssets[Index(kind)].animation);
    Animation timing;assert(timing.Measure(*data));assert(timing.outFrame>0&&timing.inFrame>timing.outFrame);
    assert(timing.Frame(*data,Phase::NeedAmmo,0,100000)==timing.outFrame);
    assert(timing.Frame(*data,Phase::Carrying,1000,1000)==timing.outFrame);
    assert(timing.Frame(*data,Phase::WaitingNative,1000,1000)==timing.inFrame);
    assert(timing.Frame(*data,Phase::WaitingNative,0,100000)==float(data->frames-1));
    armbind::Mat bind;assert(ClipBind(*data,bind));
    float native[16*12]{},changed[16*12];const auto id=Identity();
    for(unsigned slot=0;slot<data->slots;++slot)armbind::ToGpu(id,reinterpret_cast<float(*)[4]>(native+slot*12));
    assert(Palette(*data,timing.outFrame,true,native,data->slots*3,changed));
    assert(!std::memcmp(native+data->root*12,changed+data->root*12,12*sizeof(float)));
    auto beforeReload=Identity();beforeReload.m[3][0]=8;beforeReload.m[3][1]=-3;beforeReload.m[3][2]=11;
    float parent[16*12],heldResult[16*12],heldGpu[3][4];
    armbind::ToGpu(beforeReload,heldGpu);
    assert(ParentPalette(*data,native,data->slots*3,&beforeReload,parent));
    assert(Palette(*data,timing.inFrame,false,parent,data->slots*3,heldResult));
    assert(!std::memcmp(heldResult+data->root*12,heldGpu,12*sizeof(float)));
    assert(std::memcmp(native+data->root*12,parent+data->root*12,12*sizeof(float))!=0);
    for(unsigned slot=0;slot<data->slots;++slot)if(slot!=data->root)
        assert(!std::memcmp(native+slot*12,parent+slot*12,12*sizeof(float)));
    assert(ParentPalette(*data,native,data->slots*3,nullptr,parent));
    assert(!std::memcmp(native,parent,data->slots*12*sizeof(float))); // end/cancel = unmodified parent
    const unsigned clip=ClipSlot(kind);
    armbind::Mat collapsed;assert(armbind::FromGpu(reinterpret_cast<float(*)[4]>(changed+clip*12),collapsed));
    const auto a=Transform({1,2,3},collapsed),b=Transform({-10,4,-7},collapsed);
    assert(a.x==b.x&&a.y==b.y&&a.z==b.z); // every rigid clip triangle becomes degenerate
    float untouched[16*12];for(float& v:untouched)v=4567;
    assert(!Palette(*data,0,true,native,data->slots*3-1,untouched));for(float v:untouched)assert(v==4567);
    // Independent original joint -> LocalToWorld -> controller C -> XR centre
    // roundtrip, with nontrivial bind orientation, yaw, scale and sitting height.
    armbind::Mat inverseBind;assert(skeletal::Inverse(inverseBind.m,bind.m));
    for(int i=0;i<1440;++i){
        const float cy=float(i)*.013f,by=float(i)*-.023f,units=40.f+float(i%61);
        const hipknife::V centre{-.4f,.2f+float(i%17)*.1f,.9f},target{.3f,centre.y-.1f,-.5f};
        const auto world=vrpose::TrackingDeltaToWorldUU({target.x-centre.x,target.y-centre.y,target.z-centre.z},cy,by,units);
        auto goal=Identity();goal.m[3][0]=world.x;goal.m[3][1]=world.y;goal.m[3][2]=world.z;
        auto c=Identity();c.m[0][0]=c.m[1][1]=std::cos(.37f);c.m[0][1]=std::sin(.37f);c.m[1][0]=-std::sin(.37f);
        c.m[3][0]=.7f;c.m[3][1]=4.f;c.m[3][2]=2.f;
        armbind::Mat invC,tmp,local;assert(skeletal::Inverse(invC.m,c.m));
        skeletal::Multiply(tmp.m,inverseBind.m,goal.m);skeletal::Multiply(local.m,tmp.m,invC.m);
        hipknife::V output{};armbind::Mat tracking;
        assert(Socket(*data,native,local,c,centre,cy,by,units,output,&tracking));
        assert(hipknife::Length(output-target)<.00002f);
        // Original cache metres -> native bind/skin/local/C -> XR must equal
        // the affine carried mesh path, including handedness and world scale.
        const hipknife::V vertex{.05f,.1f,-.07f};
        auto worldPoint=Transform(Transform(Transform(vertex*52.5f,bind),local),c);
        const float forward=(worldPoint.x*std::cos(by)+worldPoint.y*std::sin(by))/units;
        const float right=(-worldPoint.x*std::sin(by)+worldPoint.y*std::cos(by))/units;
        const hipknife::V expected{centre.x-std::sin(cy)*forward+std::cos(cy)*right,
            centre.y+worldPoint.z/units,centre.z-std::cos(cy)*forward-std::sin(cy)*right};
        assert(hipknife::Length(Transform(vertex,tracking)-expected)<.00002f);
        const hipknife::Pose aim{{},centurionclip::Degrees(20,30,10)},hand{{-.3f,.8f,-.2f},centurionclip::Degrees(-15,-65,80)};
        armbind::Mat carried;assert(HandMatrix(tracking,aim,hand,vertex,carried));
        assert(hipknife::Length(Transform(vertex,carried)-(hand.p+hipknife::Rotate(hand.q,{0,0,-.06f})))<.00002f);
        for(unsigned axis=0;axis<3;++axis){
            const float before=hipknife::Length({tracking.m[axis][0],tracking.m[axis][1],tracking.m[axis][2]});
            const float after=hipknife::Length({carried.m[axis][0],carried.m[axis][1],carried.m[axis][2]});
            assert(std::fabs(before-after)<.00001f);
        }
    }
    std::printf("PASS original %s: palette collapse/root restore; measured animation %.0f -> %.0f; 1440 socket roundtrips\n",stem.c_str(),timing.outFrame,timing.inFrame);
}
static void TestBridge(){
    for(const auto& profile:weaponreloadproto::profiles){
        weaponreloadproto::State state;weaponreloadproto::Frame f;weaponreloadproto::Calibration config;
        config.ready=true;config.revision=1;config.gesture.socketRadius=.16f;config.gesture.pullDistance=.30f;
        f.enabled=f.liveIdentityVerified=true;f.kind=profile.kind;f.objectName=profile.objectName;f.meshWedges=profile.meshWedges;
        auto& in=f.gesture;in.context=in.supported=in.tracked=in.ammoFresh=true;
        in.pawn=20;in.weapon=uint32_t(profile.kind)+100;in.clip=0;in.reserve=19;in.capacity=8;
        in.socket={.2f,1,-.4f};in.pouch=weaponreloadproto::Supply({0,1.2f,0},.7f,config.shoulder).p;
        centurionnative::Gate gate;centurionreload::NativeTap tap;hipknife::TriggerGate fire;
        unsigned intents=0,nativeCalls=0;bool firing=false;
        auto tick=[&](bool circle,bool grip,hipknife::V hand){
            in.nowMs+=20;in.grip=grip;in.hand=hand;in.firing=firing;f.magazineReleaseDown=circle;
            const auto result=weaponreloadproto::Update(state,f,config);
            gate.Publish({true,in.pawn,in.weapon,in.nowMs});
            if(result.reloadIntent){++intents;assert(gate.Arm(in.pawn,in.weapon,in.nowMs));tap.Update(true,true,in.nowMs);}
            return result;
        };
        tick(false,false,in.socket);
        assert(gate.Evaluate(2,in.weapon,in.pawn,in.weapon,in.nowMs)==centurionnative::Decision::Block); // AUTO blocked
        assert(tick(true,false,in.socket).ejectMagazine);firing=true;
        assert(fire.Update(Active(state.transaction.phase),1)==0); // held R2 must not abort/reappear
        assert(tick(false,true,in.pouch).reservePhysicalLeft);
        assert(tick(false,true,in.socket).reservePhysicalLeft);
        assert(tick(false,false,in.socket).reloadIntent&&intents==1);
        assert(!tap.Update(true,false,in.nowMs)); // neutral edge
        assert(tap.Update(true,false,in.nowMs+20)); // ordinary XInput reload tap
        if(gate.Evaluate(2,in.weapon,in.pawn,in.weapon,in.nowMs+20)==centurionnative::Decision::Permit){
            ++nativeCalls;in.clip=8;in.reserve=11; // OWN fake inventory producer, NEVER runtime writes
        }
        assert(gate.Evaluate(2,in.weapon,in.pawn,in.weapon,in.nowMs+20)==centurionnative::Decision::Block);
        assert(tick(false,false,in.socket).event==weaponreloadproto::Event::Acknowledged);
        assert(nativeCalls==1&&intents==1&&in.clip+in.reserve==19);
        assert(fire.Update(false,1)==0);assert(fire.Update(false,0)==0);assert(fire.Update(false,1)==1);
        gate.Clear();assert(gate.Evaluate(2,in.weapon,in.pawn,in.weapon,in.nowMs)==centurionnative::Decision::Forward);
    }
    Witness w{true,Kind::Volk,1,2,100,{.2f,1,-.4f}};
    assert(w.Current(Kind::Volk,1,2,250));assert(!w.Current(Kind::Volk,1,2,251));
    assert(!w.Current(Kind::Valkyrie,1,2,110));assert(!w.Current(Kind::Volk,1,3,110));
    for(const auto& s:weaponreloadanim::specs){
        assert(DrawPart(s.kind,0,0,s.vertices,0,s.triangles)==Part::Body);
        assert(DrawPart(s.kind,0,0,s.vertices,0,s.triangles+1)==Part::None);
        assert(DrawPart(s.kind,1,0,s.vertices,0,s.triangles)==Part::None);
        assert(DrawPart(s.kind,0,1,s.vertices,0,s.triangles)==Part::None);
        if(s.kind!=Kind::Volk){const unsigned first=s.kind==Kind::Valkyrie?11378:11292,extra=s.kind==Kind::Valkyrie?8:168;
            assert(DrawPart(s.kind,0,first,s.vertices-first,s.triangles*3,extra)==Part::Attachment);
            assert(DrawPart(s.kind,0,0,s.vertices,s.triangles*3,extra)==Part::Attachment);
            assert(DrawPart(s.kind,0,first,s.vertices-first,s.triangles*3+3,extra)==Part::None);
        }
        assert(ReadableUsage(true,s.indexCount*2,true,520,8)==512);
        assert(ReadableUsage(false,s.indexCount*2,true,520,8)==520);
        assert(ReadableUsage(true,s.indexCount*2+2,true,520,8)==520);
        assert(ReadableUsage(true,s.indexCount*2,false,520,8)==520);
    }
    puts("PASS native bridge: Circle/physical-left/insert -> ONE permit + XInput -> conserved ACK, auto block, trigger rearm, stale/foreign witness");
}
int main(int argc,char** argv){
    TestBridge();assert(argc==2);for(const auto& s:weaponreloadanim::specs)TestOriginal(s,argv[1]);
    puts("L1 OWN fixtures only, NOT live inventory or physical alignment proof");
}
