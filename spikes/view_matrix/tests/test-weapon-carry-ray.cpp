#include "../weapon_reload_carry.h"
#include "../weapon_trace_math.h"
#include "../weapon_muzzle_geometry.h"
#include "../arm_natural.h"
#include <cassert>
#include <cstdio>
static armbind::Mat Identity(){armbind::Mat m{};for(unsigned i=0;i<4;++i)m.m[i][i]=1;return m;}
int main(){
    using namespace weaponreload;
    using weaponcarry::WristSource;
    assert(weaponcarry::Source(Kind::Volk,true,false,true,false,true,9)==WristSource::Tmd);
    assert(weaponcarry::Source(Kind::Volk,true,true,false,true,true,9)==WristSource::NoTmd);
    assert(weaponcarry::Source(Kind::Valkyrie,true,false,true,false,true,9)==WristSource::None);
    assert(weaponcarry::Source(Kind::Kasimov,true,false,true,false,true,9)==WristSource::None);
    assert(weaponcarry::Source(Kind::Volk,false,false,true,false,true,9)==WristSource::None);
    assert(weaponcarry::Source(Kind::Volk,true,false,true,false,false,9)==WristSource::None);
    assert(weaponcarry::Source(Kind::Volk,true,false,true,false,true,51)==WristSource::None);
    const auto& tmd=armnatural::TmdBind();assert(tmd.ready);
    for(int turn=0;turn<2;++turn){
        auto skin=Identity(),chain=Identity(),world=Identity(),tracked=Identity(),expected=Identity();
        if(turn){skin.m[0][0]=0;skin.m[0][1]=1;skin.m[1][0]=-1;skin.m[1][1]=0;}
        chain.m[3][2]=7;world.m[3][0]=13;
        float gpu[3][4];armbind::ToGpu(skin,gpu);
        assert(weaponcarry::WristTracking(tmd.bind[9],gpu,chain,world,tracked));
        armbind::Mat a,inverse;assert(skeletal::Inverse(inverse.m,world.m));
        skeletal::Multiply(a.m,tmd.bind[9].m,skin.m);
        skeletal::Multiply(expected.m,a.m,chain.m);
        skeletal::Multiply(a.m,expected.m,inverse.m);
        for(unsigned row=0;row<4;++row)for(unsigned col=0;col<4;++col)
            assert(std::fabs(tracked.m[row][col]-a.m[row][col])<.0001f);
    }
    for(unsigned i=0;i<4000;++i){
        const float x=float(i%19)*.3f;
        float a[]{x,2,-3},b[]{x+.1f,2.2f,-2.9f},offset[]{3,-4,.3f};
        float direction[3];for(unsigned k=0;k<3;++k)direction[k]=b[k]-a[k];
        assert(weapontrace::Translate(a,b,offset));
        for(unsigned k=0;k<3;++k)assert(std::fabs((b[k]-a[k])-direction[k])<.000002f);
    }
    float a[]{1,2,3},b[]{4,5,6},bad[]{0,NAN,0};
    assert(!weapontrace::Translate(a,b,bad));assert(a[0]==1&&b[0]==4);
    auto wrist=Identity();for(unsigned k=0;k<3;++k)wrist.m[k][k]=1.f/52.5f;
    wrist.m[3][0]=.4f;wrist.m[3][1]=1.2f;wrist.m[3][2]=-.6f;
    for(const auto& spec:weaponreloadanim::specs){
        armbind::Mat mount,model;assert(weaponcarry::Mount(spec.kind,mount));
        assert(weaponcarry::Matrix(spec.kind,wrist,model));
        const hipknife::V cache{.02f,-.03f,.07f};
        const auto expected=Transform(Transform({cache.x*52.5f,-cache.y*52.5f,cache.z*52.5f},mount),wrist);
        assert(hipknife::Length(Transform(cache,model)-expected)<.000001f);
        weaponcarry::Wrist witness{true,spec.kind,3,4,70,100,wrist};
        assert(witness.Current(spec.kind,3,4,70,250));
        assert(!witness.Current(spec.kind,3,4,71,150));
        assert(!witness.Current(spec.kind,3,4,70,251));
        assert(!witness.Current(spec.kind,3,5,70,150));
        auto data=weaponreloadanim::Data{};data.ready=true;data.nativeSpace=true;
        data.kind=spec.kind;data.rootBind=Identity();data.rootBind.m[3][1]=7;
        auto skin=Identity(),local=Identity(),controller=Identity();
        skin.m[3][0]=2;local.m[3][2]=3;controller.m[3][0]=11;
        hipknife::V source,result;assert(weaponmuzzle::RootPoint(spec.kind,source));
        assert(weaponmuzzle::Position(data,skin,local,controller,result));
        assert(hipknife::Length(result-(source+hipknife::V{13,7,3}))<.00001f);
        data.nativeSpace=false;assert(!weaponmuzzle::Position(data,skin,local,controller,result));
    }
    armbind::Mat out;assert(!weaponcarry::Mount(Kind::None,out));
    weaponmuzzle::Witness muzzle{true,Kind::Valkyrie,3,4,100,{1,2,3}};
    assert(muzzle.Current(3,4,250));assert(!muzzle.Current(3,4,251));assert(!muzzle.Current(3,5,120));
    puts("L1 PASS: 4000 translated rays, three native wrist mounts, TMD/Volk source and wrist rotations, muzzle chain, stale/foreign identities");
}
