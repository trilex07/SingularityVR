#include "../dual_hand_action_pose.h"
#include "../arm_natural.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <limits>
using dualaction::Mat;
static Mat Identity(){Mat a;for(int i=0;i<4;++i)a.m[i][i]=1;return a;}
static Mat Pose(float angle,float x,float y,float z){Mat a=Identity();a.m[0][0]=a.m[1][1]=std::cos(angle);
    a.m[0][1]=std::sin(angle);a.m[1][0]=-std::sin(angle);a.m[3][0]=x;a.m[3][1]=y;a.m[3][2]=z;return a;}
static float Error(const Mat& a,const Mat& b){float e=0;for(int i=0;i<4;++i)for(int j=0;j<4;++j)e=fmaxf(e,fabsf(a.m[i][j]-b.m[i][j]));return e;}
static Mat Inverse(const Mat& a){Mat r;assert(skeletal::Inverse(r.m,a.m));return r;}
static void Skin(const Mat& bind,const Mat& joint,float out[3][4]){
    armbind::ToGpu(dualaction::Product(Inverse(bind),joint),out);
}
int main(int argc,char**argv){
    const auto& b=armnatural::TmdBind();assert(b.ready);
    dualaction::Context c{true,true,true,true,false,false,false,false,true};
    assert(dualaction::Eligible(c));
    auto blocked=c;blocked.enabled=false;assert(!dualaction::Eligible(blocked));
    blocked=c;blocked.support=true;assert(!dualaction::Eligible(blocked));
    blocked=c;blocked.carrying=true;assert(!dualaction::Eligible(blocked));
    blocked=c;blocked.nativeAnimation=true;assert(!dualaction::Eligible(blocked));
    blocked=c;blocked.tracked=false;assert(!dualaction::Eligible(blocked));
    blocked=c;blocked.rightHanded=false;assert(!dualaction::Eligible(blocked));
    blocked=c;blocked.gameplay=false;assert(!dualaction::Eligible(blocked));
    blocked=c;blocked.holstered=true;assert(!dualaction::Eligible(blocked));
    float maximum=0;
    for(unsigned gun:{5799u,11387u,8297u,11389u})for(float angle:{-1.4f,0.f,1.1f}){
        // Native transition versus independent, moving controller. Include the
        // component's 0.75 scale seen in the historical final wrist receipts.
        const Mat local=Pose(.37f,3,-1,-149),controller=Pose(angle,-15,27,-8);
        const Mat transition=Pose(-angle*.7f,17,-26,13);
        Mat right=dualaction::Product(*dualhandreference::Right(gun),transition);
        Mat left=dualaction::Product(dualhandreference::Left,Pose(angle*.4f,3,8,-20));
        for(int i=0;i<4;++i)for(int j=0;j<3;++j){left.m[i][j]*=.75f;right.m[i][j]*=.75f;}
        float ls[3][4],rs[3][4];Skin(b.bind[9],left,ls);Skin(b.bind[51],right,rs);
        dualaction::Frame f;const dualaction::Key key{9,1,2,gun};
        assert(f.Capture(key,local,b.bind[9],ls,b.bind[51],rs));
        assert(f.Current(key));assert(!f.Current({10,1,2,gun}));assert(!f.Current({9,1,3,gun}));
        for(bool aim:{false,true}){
            Mat C=controller;assert(f.Apply(key,aim,C.m));
            const Mat& joint=aim?right:left;Mat ref;
            assert(dualaction::ReferenceWithScale(joint,aim?*dualhandreference::Right(gun):dualhandreference::Left,ref));
            const Mat expected=dualaction::Product(dualaction::Product(ref,local),controller);
            const Mat actual=dualaction::Product(dualaction::Product(joint,local),C);
            maximum=fmaxf(maximum,Error(expected,actual));assert(Error(expected,actual)<.0002f);
            // A live recoil/hinge descendant is NOT frozen: its current
            // relative transform survives the identical common-parent delta.
            const Mat child=Pose(.19f,2,0,-1),animated=dualaction::Product(child,joint);
            const Mat childActual=dualaction::Product(dualaction::Product(animated,local),C);
            assert(Error(childActual,dualaction::Product(child,expected))<.0002f);
            // Existing LocalDeltaPair + actual camera ride applies it ONCE.
            float dl[4][4],dr[4][4],changed[3][4];
            const Mat ride=Pose(-.23f,7,2,-5); // different actual draw owner
            assert(skeletal::LocalDeltaPair(dl,dr,local.m,ride.m,C.m,C.m));
            skeletal::TransformBone(changed,aim?rs:ls,aim?dr:dl);
            Mat skin;armbind::FromGpu(changed,skin);
            Mat final=dualaction::Product(dualaction::Product(dualaction::Product(aim?b.bind[51]:b.bind[9],skin),local),ride);
            assert(Error(final,expected)<.0002f);
            for(float eye:{-.032f,.032f}){
                Mat projection=Pose(0,eye,0,0);
                assert(Error(dualaction::Product(final,projection),dualaction::Product(expected,projection))<.0002f);
            }
        }
        Mat stale=controller;assert(!f.Apply({10,1,2,gun},true,stale.m));assert(Error(stale,controller)==0);
        rs[0][0]=std::numeric_limits<float>::quiet_NaN();assert(!f.Capture(key,local,b.bind[9],ls,b.bind[51],rs));
        assert(!f.Current(key));
    }
    Mat invalid=Identity(),out;
    invalid.m[0][0]=2;assert(!dualaction::ReferenceWithScale(invalid,dualhandreference::Left,out));
    invalid=Identity();invalid.m[1][0]=.2f;assert(!dualaction::ReferenceWithScale(invalid,dualhandreference::Left,out));
    invalid=Identity();invalid.m[0][0]=-1;assert(!dualaction::ReferenceWithScale(invalid,dualhandreference::Left,out));
    invalid=Identity();invalid.m[2][2]=0;assert(!dualaction::ReferenceWithScale(invalid,dualhandreference::Left,out));
    assert(!dualhandreference::Right(12444)); // Seeker baseline is unchanged.
    unsigned records=0;float replayMax=0;
    if(argc==2){
        // Private archived final material-0 matrices; this exercises the real
        // C++ decoder/normalizer, not a separate Python implementation.
        std::ifstream in(argv[1]);unsigned count;in>>count;assert(in&&count<10000);
        for(unsigned i=0;i<count;++i){float skin[3][4];Mat local;
            for(auto& row:skin)for(float& v:row)in>>v;
            for(auto& row:local.m)for(float& v:row)in>>v;
            assert(in);Mat correction,current,ref;
            assert(dualaction::Correction(b.bind[51],skin,local,dualhandreference::Right8297,correction));
            assert(armbind::CurrentJoint(b.bind[51],skin,current));
            assert(dualaction::ReferenceWithScale(current,dualhandreference::Right8297,ref));
            float error=Error(dualaction::Product(dualaction::Product(current,local),correction),dualaction::Product(ref,local));
            assert(error<.001f);replayMax=fmaxf(replayMax,error);++records;
        }
    }
    printf("PASS action-pose x86: 4 weapons, 3 rotations, 2 hands/eyes; child animation, scale, single application, stale/invalid/OFF/support gates; max=%g; historical records=%u max=%g\n",maximum,records,replayMax);
}
