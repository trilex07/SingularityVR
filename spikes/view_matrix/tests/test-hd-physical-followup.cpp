#include "../weapon_muzzle_geometry.h"
#include "../weapon_reload_carry.h"
#include "../kasimov_support_geometry.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <vector>
using armbind::Mat;
static Mat Id(){Mat m{};for(unsigned i=0;i<4;++i)m.m[i][i]=1;return m;}
static Mat Product(const Mat& a,const Mat& b){Mat c;skeletal::Multiply(c.m,a.m,b.m);return c;}
static std::vector<unsigned char> Read(const char* path){
    std::ifstream f(path,std::ios::binary|std::ios::ate);assert(f.good());const auto n=f.tellg();
    assert(n>0&&n<4*1024*1024);std::vector<unsigned char> b((size_t)n);f.seekg(0);f.read((char*)b.data(),n);assert(f.good());return b;
}
int main(int argc,char** argv){
    assert(argc==3);using namespace weaponreload;
    auto animation=Read(argv[1]),mesh=Read(argv[2]);
    auto d=std::make_unique<weaponreloadanim::Data>();
    assert(weaponreloadanim::Decode(animation.data(),animation.size(),Kind::Kasimov,*d)&&weaponreloadanim::ToNative(*d));
    assert(!memcmp(mesh.data(),"SVKS",4)&&weaponreloadanim::U32(mesh.data()+16)==11389);
    unsigned mixed=0,cases=0;float worst=0,oldWorst=0;
    for(unsigned frame:{0u,60u,120u,180u,238u})for(float angle:{-.9f,.65f}){
        Mat nativeRoot=Id(),held=Id(),parts[8];
        nativeRoot.m[0][0]=nativeRoot.m[1][1]=cosf(angle);
        nativeRoot.m[0][1]=sinf(angle);nativeRoot.m[1][0]=-sinf(angle);
        nativeRoot.m[3][0]=11;nativeRoot.m[3][1]=-8;held.m[3][2]=6;
        assert(weaponreloadanim::Sample(*d,float(frame),parts));
        float native[72],out[72],old[72];
        for(unsigned slot=0;slot<6;++slot)armbind::ToGpu(nativeRoot,reinterpret_cast<float(*)[4]>(native+slot*12));
        for(unsigned t=0;t<d->tracks;++t){auto m=Product(parts[t],nativeRoot);armbind::ToGpu(m,reinterpret_cast<float(*)[4]>(native+d->target[t]*12));}
        memcpy(old,native,sizeof old);armbind::ToGpu(held,reinterpret_cast<float(*)[4]>(old));
        assert(ParentPalette(*d,native,18,&held,out));
        Mat inverse;assert(skeletal::Inverse(inverse.m,nativeRoot.m));const auto delta=Product(inverse,held);
        unsigned count=0;
        for(unsigned v=0;v<11292;++v){
            const auto* p=mesh.data()+28+v*32;hipknife::V pos;memcpy(&pos,p,12);
            hipknife::V before{},after{},broken{};unsigned influences=0;
            for(unsigned b=0;b<4;++b)if(p[24+b]){
                ++influences;const unsigned slot=p[20+b];assert(slot<6);const float w=p[24+b]/255.f;
                Mat a,c,o;assert(armbind::FromGpu(reinterpret_cast<float(*)[4]>(native+slot*12),a));
                assert(armbind::FromGpu(reinterpret_cast<float(*)[4]>(out+slot*12),c));
                assert(armbind::FromGpu(reinterpret_cast<float(*)[4]>(old+slot*12),o));
                before=before+Transform(pos,a)*w;after=after+Transform(pos,c)*w;broken=broken+Transform(pos,o)*w;
            }
            const auto expected=Transform(before,delta);
            worst=std::max(worst,hipknife::Length(after-expected));
            if(influences>1){++count;oldWorst=std::max(oldWorst,hipknife::Length(broken-expected));}
        }
        assert(count>143);mixed=count;++cases;
        assert(ParentPalette(*d,native,18,nullptr,out)&&!memcmp(native,out,sizeof native));
        // Independent lens section must end at the same cog*local frame.
        auto bodyLocal=Id(),lensLocal=Id();bodyLocal.m[3][0]=17;lensLocal.m[3][2]=3;
        const auto rootLocal=Product(Product(kasimovsupport::RootBind,held),bodyLocal);
        Mat lensSkin;assert(kasimovsupport::LensPalette(rootLocal,lensLocal,lensSkin));
        auto singular=lensLocal;singular.m[0][0]=0;Mat refused;
        assert(!kasimovsupport::LensPalette(rootLocal,singular,refused));
        const hipknife::V point{0,-11.1806386f,-3.49776761f};
        const auto a=Transform(Transform(point,kasimovsupport::RootBind),Product(lensSkin,lensLocal));
        const auto b=Transform(point,rootLocal);assert(hipknife::Length(a-b)<.0001f);
    }
    assert(worst<.0001f&&oldWorst>1.f);
    // Exercise the real FSM: controller is outside tolerance, source-derived
    // clip origin is inside. Missing/foreign wrist never creates a permit.
    for(unsigned mode=0;mode<3;++mode){
        weaponreloadproto::State state;weaponreloadproto::Frame f;weaponreloadproto::Calibration c;
        c.ready=true;c.revision=1;f.enabled=f.liveIdentityVerified=true;f.kind=Kind::Kasimov;
        f.objectName=weaponreloadproto::Find(f.kind)->objectName;f.meshWedges=11389;
        auto& g=f.gesture;g.context=g.supported=g.tracked=g.ammoFresh=true;
        g.pawn=1;g.weapon=2;g.capacity=5;g.clip=0;g.reserve=10;g.nowMs=1000;
        g.socket={0,1,0};g.pouch={0,1,.7f};g.hand=g.socket;
        weaponreloadproto::Update(state,f,c);f.magazineReleaseDown=true;++g.nowMs;
        assert(weaponreloadproto::Update(state,f,c).event==weaponreloadproto::Event::Ejected);
        f.magazineReleaseDown=false;g.hand=g.pouch;g.grip=true;++g.nowMs;
        assert(weaponreloadproto::Update(state,f,c).event==weaponreloadproto::Event::GrabFresh);
        g.hand={.25f,1,0};g.grip=false;++g.nowMs;
        f.measuredInsertion=true;f.insertionValid=mode!=1;f.insertionPoint=mode==2?g.hand:g.socket;
        const auto result=weaponreloadproto::Update(state,f,c);
        assert(result.reloadIntent==(mode==0));
    }
    assert(weaponmuzzle::CenturionMuzzle.y<0);
    // The old point cannot be on the same rendered barrel under any rigid C.
    for(float angle:{-.8f,0.f,1.3f}){
        auto c=Id();c.m[0][0]=c.m[1][1]=cosf(angle);c.m[0][1]=sinf(angle);c.m[1][0]=-sinf(angle);
        auto old=weaponmuzzle::CenturionMuzzle;old.y=-old.y;
        assert(fabsf(hipknife::Length(Transform(old,c)-Transform(weaponmuzzle::CenturionMuzzle,c))-229.3914f)<.0001f);
    }
    printf("PASS actual Kasimov VB: %u mixed wedges x %u palettes; error=%g UU, old root-only error=%g UU; lens, measured insertion FSM, Centurion origin\n",mixed,cases,worst,oldWorst);
}
