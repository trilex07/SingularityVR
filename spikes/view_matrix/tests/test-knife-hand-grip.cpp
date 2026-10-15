#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cassert>
#include <cstdio>
#include "../glove_native.h"

static void ClipTests(){
    using namespace knifehand;
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
}
static void CapturedShader(const char* path,bool shadow){
    std::vector<unsigned char> bytes;assert(gloves::Read(path,bytes,65536)&&bytes.size()%4==0);
    std::vector<std::uint32_t> words(bytes.size()/4);std::memcpy(words.data(),bytes.data(),bytes.size());
    const auto layout=skeletal::ReadLayout(words.data(),(unsigned)words.size());
    assert(layout.bones==5&&layout.boneRegs==225&&layout.local==230&&layout.shadow==shadow);
    assert(layout.viewProjection==(shadow?-1:0));
}
int main(int argc,char** argv){
    ClipTests();
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
