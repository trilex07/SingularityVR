#include "../tmd_aim_policy.h"
#include <cassert>
#include <cstdio>
#include <limits>
#include <initializer_list>
static bool near(float a,float b){return std::fabs(a-b)<0.0001f;}
int main(){
    tmdaim::Pose p;p.pawn=123;p.time=100;
    for(unsigned i=0;i<4;++i)p.matrix[i][i]=1;
    p.anchor[0]=10;p.camera[0]=100;p.forward[0]=1;
    float o[3]{},d[3]{};
    assert(tmdaim::Ray(p,o,d)&&near(o[0],110)&&near(d[0],1));
    // Left hand +90deg yaw and pivot translation, not the right gun's axis.
    p.matrix[0][0]=0;p.matrix[0][1]=1;p.matrix[1][0]=-1;p.matrix[1][1]=0;
    p.matrix[3][0]=7;p.matrix[3][1]=-8;
    assert(tmdaim::Ray(p,o,d)&&near(o[0],107)&&near(o[1],2)&&near(d[1],1));
    // Uniform mesh scaling must not increase cast range via direction magnitude.
    p.matrix[0][1]=2;p.matrix[1][0]=-2;p.matrix[2][2]=2;
    assert(tmdaim::Ray(p,o,d)&&near(d[1],1));
    p.matrix[0][1]=std::numeric_limits<float>::quiet_NaN();
    o[0]=91;d[0]=92;
    assert(!tmdaim::Ray(p,o,d)&&o[0]==91&&d[0]==92);
    assert(!tmdaim::Ray(p,nullptr,d)&&!tmdaim::Ray(p,d,d));
    assert(tmdaim::Current(p,123,250)&&!tmdaim::Current(p,123,251));
    assert(!tmdaim::Current(p,124,101)&&!tmdaim::Current(p,123,99));
    assert(tmdaim::Eligible(true,true,true,true,true,false));
    for(unsigned bit=0;bit<5;++bit){bool gate[]{true,true,true,true,true};gate[bit]=false;
        assert(!tmdaim::Eligible(gate[0],gate[1],gate[2],gate[3],gate[4],false));}
    assert(!tmdaim::Eligible(true,true,true,true,true,true));
    for(int yaw=-180;yaw<=180;yaw+=15)for(int pitch=-90;pitch<=90;pitch+=15)
    for(float scale:{0.5f,1.f,2.f}) {
        const float y=yaw*0.01745329252f,q=pitch*0.01745329252f;
        tmdaim::Pose v;v.anchor[0]=31;v.anchor[1]=-9;v.anchor[2]=-14;
        v.camera[0]=1000;v.camera[1]=-2000;v.camera[2]=100;v.forward[0]=1;
        const float r[3][3]{{cosf(q)*cosf(y),cosf(q)*sinf(y),sinf(q)},
            {-sinf(y),cosf(y),0},{-sinf(q)*cosf(y),-sinf(q)*sinf(y),cosf(q)}};
        const float hand[3]{20,-30,50};
        for(unsigned j=0;j<3;++j) {
            v.matrix[3][j]=hand[j];
            for(unsigned i=0;i<3;++i){v.matrix[i][j]=r[i][j]*scale;
                v.matrix[3][j]-=v.anchor[i]*v.matrix[i][j];}
        }
        assert(tmdaim::Ray(v,o,d));
        for(unsigned j=0;j<3;++j){assert(near(o[j],v.camera[j]+hand[j]));assert(near(d[j],r[0][j]));}
    }
    std::puts("PASS TMD ray: exact row transform, scale, invalid/stale/foreign/gun-nested gates");
}
