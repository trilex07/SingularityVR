#include "../handedness.h"
#include "../weapon_wheel.h"
#include "../weapon_reload_prototypes.h"
#include "../hand_anchor_math.h"
#include "../glove_isolate.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>

static void Near(float a,float b){assert(std::fabs(a-b)<.0003f);}
static armbind::Mat Identity(){armbind::Mat m{};for(unsigned i=0;i<4;++i)m.m[i][i]=1;return m;}
int main(){
    char path[128],back[128];
    struct Binding{const char* canonical;const char* mirrored;};
    const Binding bindings[]={
        {"/user/hand/right/input/trigger/value","/user/hand/left/input/trigger/value"},
        {"/user/hand/right/input/squeeze/value","/user/hand/left/input/squeeze/value"},
        {"/user/hand/left/input/trigger/value","/user/hand/right/input/trigger/value"},
        {"/user/hand/right/input/a/click","/user/hand/left/input/x/click"},
        {"/user/hand/right/input/b/click","/user/hand/left/input/y/click"},
        {"/user/hand/left/input/x/click","/user/hand/right/input/a/click"},
        {"/user/hand/left/input/y/click","/user/hand/right/input/b/click"},
        {"/user/hand/left/input/menu/click","/user/hand/left/input/menu/click"}};
    for(const auto& b:bindings){
        assert(!std::strcmp(handedness::MirrorPath(false,b.canonical,path,sizeof(path)),b.canonical));
        const auto result=handedness::MirrorPath(true,b.canonical,path,sizeof(path));
        assert(!std::strcmp(result,b.mirrored));
        assert(!std::strcmp(handedness::MirrorPath(true,result,back,sizeof(back)),b.canonical));
    }
    assert(handedness::MirrorPath(true,bindings[0].canonical,path,2)==bindings[0].canonical);
    for(unsigned i=0;i<8;++i)assert(handedness::MirrorGameplayButton(bindings[i].canonical)==(i<3));
    for(bool left:{false,true}){
        const int aim=handedness::Aim(left),off=handedness::Off(left);
        assert(aim!=off&&aim+off==1);
        assert(gloveisolate::TmdHardwareTrackedSide(off)==off&&gloveisolate::TmdHardwareDelta(off)==0);
        weaponwheel::State wheel;
        const auto tick=[&](bool gunStick,bool otherStick,float x=0,float y=0){
            const bool l3=left?gunStick:otherStick,r3=left?otherStick:gunStick;
            return weaponwheel::Update(wheel,handedness::WheelButton(left,l3,r3),
                handedness::WheelVeto(left,l3,r3),true,true,false,x,y,4);
        };
        assert(tick(false,false)==weaponwheel::Event::None);
        assert(tick(true,false)==weaponwheel::Event::Open);
        assert(tick(true,false,1,0)==weaponwheel::Event::SelectionChanged);
        assert(tick(false,false)==weaponwheel::Event::Commit);
        assert(tick(false,false)==weaponwheel::Event::None);
        assert(tick(true,true)==weaponwheel::Event::None); // recenter chord never opens wheel
        tick(false,false);
        assert(tick(true,false)==weaponwheel::Event::Open);
        assert(tick(true,true)==weaponwheel::Event::Cancel);
        for(int degrees=-180;degrees<=180;degrees+=15){
            const float yaw=degrees*3.14159265359f/180;
            centurionreload::Config c;c.rightShoulder=left;
            weaponreloadproto::Shoulder shoulder;shoulder.right=left;shoulder.leftExtension=.18f;
            const hipknife::V head{.3f,1.2f,-.4f};
            const auto pouch=centurionreload::Pouch(head,yaw,c);
            const auto supply=weaponreloadproto::Supply(head,yaw,shoulder);
            Near(pouch.p.x,supply.p.x);Near(pouch.p.y,supply.p.y);Near(pouch.p.z,supply.p.z);
            const hipknife::Q inverse{0,-std::sin(yaw*.5f),0,std::cos(yaw*.5f)};
            const auto body=hipknife::Rotate(inverse,supply.p-head);
            Near(body.x,left?.22f:-.22f);Near(body.y,-.12f);Near(body.z,.18f);
            const hipknife::V reach=hipknife::Rotate(supply.q,{left?.18f:-.18f,0,0});
            Near(weaponreloadproto::ShoulderDistance(supply.p+reach,supply.p,yaw,shoulder),0);
            assert(weaponreloadproto::ShoulderDistance(supply.p-reach,supply.p,yaw,shoulder)>.17f);

            // Real source bind wrists differ. Conjugate through a nontrivial
            // component and camera ride; each anatomical wrist must land on
            // its OWN controller, while the source hardware keeps its off ride.
            auto local=Identity(),ride=Identity(),controller=Identity();
            local.m[3][0]=15;local.m[3][1]=-31;ride.m[3][2]=12;
            controller.m[0][0]=controller.m[1][1]=std::cos(yaw);
            controller.m[0][1]=std::sin(yaw);controller.m[1][0]=-std::sin(yaw);
            for(int side=0;side<2;++side){
                auto bind=Identity(),skin=Identity();bind.m[3][0]=side? -49.f:49.f;
                float gpu[3][4];armbind::ToGpu(skin,gpu);
                const armik::V target{side?20.f:-20.f,7,13};
                float corrected[4][4],delta[4][4],unused[4][4];
                assert(handanchor::TargetWrist(corrected,bind,gpu,local.m,controller.m,target));
                assert(skeletal::LocalDeltaPair(delta,unused,local.m,ride.m,corrected,controller.m));
                armbind::Mat chain,a,b;
                skeletal::Multiply(a.m,bind.m,delta);skeletal::Multiply(b.m,a.m,local.m);
                skeletal::Multiply(chain.m,b.m,ride.m);
                Near(chain.m[3][0],target.x);Near(chain.m[3][1],target.y);Near(chain.m[3][2],target.z);
            }
        }
    }
    std::puts("PASS: both handedness layouts, valid mirrored XR bindings, L3/R3 wheel/chord, both shoulder yaw/capsule regions, anatomical wrist pivots through actual ride");
}
