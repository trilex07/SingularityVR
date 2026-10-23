#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cassert>
#include <cstdio>
#include "../glove_native.h"

static void ClipTests(){
    using namespace knifehand;
    JointRoute route;
    // The overlay is visited before the TMD material-0 joint in a pickup
    // frame: reserve both eyes for one native owner, not two knife copies.
    assert(route.Native(41,100,true,-1,0));
    assert(route.Native(41,101,true,100,3));
    assert(route.Native(41,102,true,101,3));
    assert(!route.fallback);
    // A permanently absent joint switches once to the standalone fallback.
    route.Reset();
    assert(route.Native(42,200,true,-1,0));
    assert(route.Native(42,201,true,-1,0));
    assert(!route.Native(42,202,true,-1,0));
    assert(route.fallback && !route.Native(42,203,true,202,3));
    assert(route.Native(43,204,true,-1,0)); // new pickup rearms native route
    assert(!route.Native(43,205,false,-1,0)); // invalid tracking permits fallback
    const Viewport vp{10,20,800,600,.1f,.9f};
    const ClipVertex base[3]={{{-.5f,-.5f,.5f,1},0,0},{{.5f,-.5f,.5f,1},1,0},{{0,.5f,.5f,1},.5f,1}};
    ClipVertex out[12];assert(ClipTriangle(base,out)==3);
    ScreenVertex s;assert(Screen(base[0],vp,s)&&s.x==210&&s.y==470&&std::fabs(s.z-.5f)<1e-6f&&s.rhw==1);
    for(unsigned which=0;which<3;++which){ClipVertex input[3];std::memcpy(input,base,sizeof(input));
        if(which==0)input[0].clip[2]=-.5f;
        if(which==1)input[0].clip[2]=2;
        if(which==2){input[0].clip[3]=-.25f;input[0].clip[2]=-.1f;}
        const unsigned n=ClipTriangle(input,out);assert(n>=3&&n<=12&&n%3==0);
        for(unsigned i=0;i<n;++i){assert(Plane(out[i],0)>-1e-5f&&Plane(out[i],1)>-1e-5f&&Plane(out[i],2)>-1e-5f);
            assert(Screen(out[i],vp,s)&&s.z>=.09999f&&s.z<=.90001f);}
    }
    ClipVertex gone[3];std::memcpy(gone,base,sizeof(gone));for(auto& p:gone)p.clip[2]=-1;
    assert(!ClipTriangle(gone,out));
    float perspective[4][4]={{1,0,0,0},{0,1,0,0},{0,0,1.01f,1},{0,0,-.01f,0}};
    assert(StandardDepth(perspective));perspective[2][2]=-.01f;assert(!StandardDepth(perspective));
    perspective[2][2]=NAN;assert(!StandardDepth(perspective));
    for(unsigned b=0;b<90;++b)assert((Digit(b)>=0)==nativegloves::Finger(b));
    for(unsigned bits=0;bits<128;++bits)for(int holder=-1;holder<=2;++holder){
        const bool on=bits&1,eligible=bits&2,ready=bits&4,visible=bits&8,tracked=bits&16,melee=bits&32,heal=bits&64;
        const bool keep=KeepTrackedMelee(on,eligible,ready,visible,holder,tracked,melee,heal);
        assert(keep==(bits==63&&(holder==0||holder==1)));
        if(!on||heal||holder<0||holder>1)assert(!keep);
        // Heal and non-knife melee retain their original display policy.
        if(heal)assert(heal||(melee&&!keep));
    }
}
static void MeleeMath(){
    using namespace knifehand;armbind::Mat ref{},bind{},delta{};
    for(unsigned i=0;i<4;++i)ref.m[i][i]=bind.m[i][i]=delta.m[i][i]=1;
    bind.m[3][0]=49;bind.m[3][1]=-4;bind.m[3][2]=12;
    // Left/right: controller rotations and .75 scale stay, even while native
    // attack spins the wrist through 180 degrees and changes its translation.
    for(unsigned side=0;side<2;++side)for(unsigned degrees=0;degrees<=180;degrees+=15){
        const float angle=(float)degrees*3.14159265f/180,sign=side?-1.f:1.f;
        armbind::Mat raw=ref;raw.m[0][0]=raw.m[1][1]=std::cos(angle);raw.m[0][1]=sign*std::sin(angle);raw.m[1][0]=-sign*std::sin(angle);
        raw.m[3][0]=7;raw.m[3][1]=14;raw.m[3][2]=3;
        delta.m[0][0]=0;delta.m[0][1]=.75f;delta.m[1][0]=-.75f;delta.m[1][1]=0;delta.m[2][2]=.75f;
        delta.m[3][0]=10;delta.m[3][1]=20;delta.m[3][2]=30;
        armbind::Mat tracked,out;skeletal::Multiply(tracked.m,raw.m,delta.m);
        assert(StabilizeMeleeWrist(out,ref,raw,tracked,bind));
        for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)assert(std::fabs(out.m[r][c]-delta.m[r][c])<1e-5f);
        for(unsigned c=0;c<3;++c){float a=out.m[3][c],b=tracked.m[3][c];
            for(unsigned k=0;k<3;++k){a+=bind.m[3][k]*out.m[k][c];b+=bind.m[3][k]*tracked.m[k][c];}
            assert(std::fabs(a-b)<1e-4f);}
    }
    MeleeReference state;float raw[3][4],tracked[3][4];armbind::ToGpu(ref,raw);armbind::ToGpu(delta,tracked);
    assert(!state.Update(123,1,0,true,raw,tracked,bind)); // no invented reference
    assert(state.Update(123,1,0,false,raw,tracked,bind));assert(state.Update(123,1,0,true,raw,tracked,bind));
    assert(!state.Update(123,2,0,true,raw,tracked,bind)); // release/regrab invalidates
    assert(state.Update(123,2,0,false,raw,tracked,bind));assert(!state.Update(124,2,0,true,raw,tracked,bind));
    assert(!state.Update(124,2,1,true,raw,tracked,bind));state.Reset();assert(!state.valid);
    // Non-identity baseline, compound axes and fail-closed output transaction.
    armbind::Mat baseline=ref,current=ref,want,actual,trackedMat;
    baseline.m[0][0]=0;baseline.m[0][2]=1;baseline.m[2][0]=-1;baseline.m[2][2]=0;
    current.m[1][1]=0;current.m[1][2]=1;current.m[2][1]=-1;current.m[2][2]=0;
    skeletal::Multiply(trackedMat.m,current.m,delta.m);skeletal::Multiply(want.m,baseline.m,delta.m);
    assert(StabilizeMeleeWrist(actual,baseline,current,trackedMat,bind));
    for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)assert(std::fabs(actual.m[r][c]-want.m[r][c])<1e-5f);
    const auto sentinel=actual;current.m[1][0]=.5f;
    assert(!StabilizeMeleeWrist(actual,baseline,current,trackedMat,bind));assert(!std::memcmp(&actual,&sentinel,sizeof(actual)));
    current=ref;current.m[0][0]=NAN;
    assert(!StabilizeMeleeWrist(actual,baseline,current,trackedMat,bind));assert(!std::memcmp(&actual,&sentinel,sizeof(actual)));
    current=ref;for(unsigned r=0;r<3;++r)current.m[r][r]=2;
    assert(!StabilizeMeleeWrist(actual,baseline,current,trackedMat,bind));assert(!std::memcmp(&actual,&sentinel,sizeof(actual)));
}
static void TmdWristMount(){
    using namespace knifehand;
    armbind::Mat no{},tmd{},knife{},joint{};
    for(unsigned i=0;i<4;++i)no.m[i][i]=tmd.m[i][i]=knife.m[i][i]=joint.m[i][i]=1;
    no.m[3][0]=48;no.m[3][1]=-7;tmd.m[3][0]=51;tmd.m[3][1]=12;
    knife.m[3][0]=52;knife.m[3][1]=-4;knife.m[3][2]=6;
    for(float angle:{0.f,.4f,-1.2f,2.4f}){
        joint.m[0][0]=joint.m[1][1]=std::cos(angle);
        joint.m[0][1]=std::sin(angle);joint.m[1][0]=-std::sin(angle);
        joint.m[3][0]=10+angle;joint.m[3][1]=15-angle;
        armbind::Mat invNo,invTmd,oldSkin,tmdSkin,oldKnife,newMount,newKnife;
        assert(skeletal::Inverse(invNo.m,no.m)&&skeletal::Inverse(invTmd.m,tmd.m));
        skeletal::Multiply(oldSkin.m,invNo.m,joint.m);
        skeletal::Multiply(tmdSkin.m,invTmd.m,joint.m);
        skeletal::Multiply(oldKnife.m,knife.m,oldSkin.m);
        assert(RetargetMount(newMount,knife,no,tmd));
        skeletal::Multiply(newKnife.m,newMount.m,tmdSkin.m);
        for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)
            assert(std::fabs(oldKnife.m[r][c]-newKnife.m[r][c])<1e-4f);
    }
    auto invalid=no;invalid.m[0][0]=0;
    armbind::Mat out{};assert(!RetargetMount(out,knife,invalid,tmd));
}
static void CapturedShader(const char* path,bool shadow){
    std::vector<unsigned char> bytes;assert(gloves::Read(path,bytes,65536)&&bytes.size()%4==0);
    std::vector<std::uint32_t> words(bytes.size()/4);std::memcpy(words.data(),bytes.data(),bytes.size());
    const auto layout=skeletal::ReadLayout(words.data(),(unsigned)words.size());
    assert(layout.bones==5&&layout.boneRegs==225&&layout.local==230&&layout.shadow==shadow);
    assert(layout.viewProjection==(shadow?-1:0));
}
int main(int argc,char** argv){
    ClipTests();MeleeMath();TmdWristMount();
    if(argc<3){std::puts("PASS joint homogeneous clipping/depth/digit math; private replay not requested");return 0;}
    std::vector<unsigned char> bytes;assert(gloves::Read(argv[1],bytes,1024));knifehand::Data data;
    assert(knifehand::Decode(bytes.data(),bytes.size(),data));
    for(unsigned offset: {0u,4u,8u,12u,20u,24u,28u}){auto bad=bytes;bad[offset]^=255;
        knifehand::Data preserved=data;assert(!knifehand::Decode(bad.data(),bad.size(),preserved));
        assert(!std::memcmp(&preserved,&data,sizeof(data)));}
    {auto bad=bytes;float nan=NAN;std::memcpy(bad.data()+96,&nan,4);assert(!knifehand::Decode(bad.data(),bad.size(),data));}
    nativegloves::Renderer native;assert(native.Load(argv[2]));
    // Releasing the assembly must use the ordinary free-hand pose again; the
    // optional per-digit path must not latch state or change the opposite hand.
    float released[135][4]{},ordinary[135][4]{},oneHand[135][4]{};
    for(unsigned s=0;s<45;++s)for(unsigned r=0;r<3;++r)
        released[s*3+r][r]=ordinary[s*3+r][r]=oneHand[s*3+r][r]=1;
    const float freeGrip[2]={0,.35f};float oneDigits[2][5];
    for(unsigned d=0;d<5;++d){oneDigits[0][d]=data.fingers[0][d];oneDigits[1][d]=freeGrip[1];}
    assert(native.Pose(oneHand,freeGrip,oneDigits));
    assert(native.Pose(released,freeGrip));assert(native.Pose(ordinary,freeGrip));
    assert(!std::memcmp(released,ordinary,sizeof(released)));
    bool changed=false;
    for(unsigned s=0;s<gloveisolate::kNoTmdCookedPaletteCount;++s){
        const unsigned bone=gloveisolate::kNoTmdCookedPalette[s];
        if(bone>=70)assert(!std::memcmp(oneHand+s*3,ordinary+s*3,12*sizeof(float)));
        else if(std::memcmp(oneHand+s*3,ordinary+s*3,12*sizeof(float)))changed=true;
    }
    assert(changed);
    float palette[135][4]{};for(unsigned s=0;s<45;++s)for(unsigned r=0;r<3;++r)palette[s*3+r][r]=1;
    const float grip[2]={0,0};assert(native.Pose(palette,grip,data.fingers));
    if(argc>=4){
        std::vector<unsigned char> golden;assert(gloves::Read(argv[3],golden,2*1024*1024)&&golden.size()>=12);
        assert(!std::memcmp(golden.data(),"SVJG",4));const unsigned n=gloves::U32(golden.data()+4),kn=gloves::U32(golden.data()+8);
        assert(golden.size()==12+std::size_t(n)*56+std::size_t(kn)*28);
        for(unsigned i=0;i<n;++i){const auto* p=golden.data()+12+i*56;float xyz[3],weights[4],expected[3];unsigned bones[4];
            std::memcpy(xyz,p,12);std::memcpy(weights,p+12,16);std::memcpy(bones,p+28,16);std::memcpy(expected,p+44,12);
            float actual[3]{};
            for(unsigned j=0;j<4;++j)if(weights[j]>0){unsigned slot=999;
                for(unsigned s=0;s<gloveisolate::kNoTmdCookedPaletteCount;++s)if(gloveisolate::kNoTmdCookedPalette[s]==bones[j])slot=s;
                assert(slot!=999);for(unsigned c=0;c<3;++c){const float* m=palette[slot*3+c];
                    actual[c]+=weights[j]*(xyz[0]*m[0]+xyz[1]*m[1]+xyz[2]*m[2]+m[3]);}}
            for(unsigned c=0;c<3;++c)assert(std::fabs(actual[c]-expected[c])<.00015f);
        }
        for(unsigned i=0;i<kn;++i){const auto* p=golden.data()+12+n*56+i*28;unsigned side;float xyz[3],expected[3];
            std::memcpy(&side,p,4);std::memcpy(xyz,p+4,12);std::memcpy(expected,p+16,12);assert(side<2);
            for(unsigned c=0;c<3;++c){const auto& m=data.mount[side].m;
                const float actual=xyz[0]*m[0][c]+xyz[1]*m[1][c]+xyz[2]*m[2][c]+m[3][c];
                assert(std::fabs(actual-expected[c])<.00015f);}
        }
        std::printf("PASS joint original-rig replay %u hand vertices + %u knife vertices (both physical sides)\n",n,kn);
    }
    if(argc>=7){CapturedShader(argv[4],false);CapturedShader(argv[5],false);CapturedShader(argv[6],true);}
    std::puts("PASS joint cache, actual captured native shader CTAB + shadow exclusion, homogeneous near/far/depth math");
}
