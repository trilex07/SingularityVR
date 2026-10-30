#include "../wrist_equipment_pose.h"
#include "../wrist_equipment_policy.h"
#include "../arm_natural.h"
#include <cassert>
#include <cstdio>
#include <limits>
static armbind::Mat Identity(){armbind::Mat a{};for(unsigned i=0;i<4;++i)a.m[i][i]=1;return a;}
int main(){
    assert(wristhud::RuntimeAllowed(true,true,false,false,false,false,false,true));
    for(unsigned gate=0;gate<8;++gate){
        bool g[]{true,true,false,false,false,false,false,true};g[gate]=!g[gate];
        assert(!wristhud::RuntimeAllowed(g[0],g[1],g[2],g[3],g[4],g[5],g[6],g[7]));
    }
    const auto& tmd=armnatural::TmdBind();assert(tmd.ready);
    unsigned cases=0;
    for(unsigned turn=0;turn<3;++turn)for(unsigned rideKind=0;rideKind<4;++rideKind){
        auto skin=Identity(),local=Identity(),ride=Identity(),world=Identity(),out=Identity();
        const float angle=turn*.6f,cs=cosf(angle),sn=sinf(angle);
        skin.m[0][0]=cs;skin.m[0][1]=sn;skin.m[1][0]=-sn;skin.m[1][1]=cs;
        local.m[3][2]=3;ride.m[3][0]=float(rideKind)*12;ride.m[3][1]=-2;
        assert(weaponreload::TrackingMap({.3f,1.6f,.1f},.4f,.3f,52.5f,world));
        float gpu[3][4];armbind::ToGpu(skin,gpu);
        assert(wristhud::FinalWrist(tmd.bind[9],gpu,local,ride,world,out));
        // Independent complete final GPU chain reference; catches omission of
        // moved=3's assembly ride and wrong order/double controller transforms.
        armbind::Mat a,b,c,inv,expected;
        assert(skeletal::Inverse(inv.m,world.m));
        skeletal::Multiply(a.m,tmd.bind[9].m,skin.m);
        skeletal::Multiply(b.m,a.m,local.m);skeletal::Multiply(c.m,b.m,ride.m);
        skeletal::Multiply(expected.m,c.m,inv.m);
        for(unsigned r=0;r<4;++r)for(unsigned k=0;k<4;++k)
            assert(fabsf(expected.m[r][k]-out.m[r][k])<1e-4f);
        // A local physical 155 mm panel must remain 155 mm in XR metres.
        const float len=sqrtf(out.m[0][0]*out.m[0][0]+out.m[0][1]*out.m[0][1]+out.m[0][2]*out.m[0][2]);
        assert(fabsf(len*.155f*52.5f-.155f)<1e-5f);++cases;
    }
    auto invalid=Identity(),out=Identity();invalid.m[0][0]=std::numeric_limits<float>::quiet_NaN();
    float gpu[3][4];armbind::ToGpu(Identity(),gpu);
    assert(!wristhud::FinalWrist(tmd.bind[9],gpu,Identity(),invalid,Identity(),out));
    printf("PASS wrist runtime gates; %u final-palette/ride/rotation/unit cases; invalid matrices refused\n",cases);
}
