#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d9.h>
#include <cassert>
#include <cstdio>
#include <chrono>
#include "../../../spikes/view_matrix/glove_native.h"

struct Buffer {
    unsigned refs=1;D3DINDEXBUFFER_DESC desc{};bool fail=false;
    Buffer(){desc.Format=D3DFMT_INDEX16;desc.Size=33024;}
    HRESULT GetDesc(D3DINDEXBUFFER_DESC* p){*p=desc;return fail?E_FAIL:S_OK;}
    void Release(){assert(refs>1);--refs;}
};
struct Device {
    Buffer original,replacement;Buffer* current=&original;
    bool getFail=false,setFail=false;unsigned calls=0;
    HRESULT GetIndices(Buffer** p){if(getFail)return E_FAIL;*p=current;++current->refs;return S_OK;}
    HRESULT SetIndices(Buffer* p){++calls;if(setFail)return E_FAIL;current=p;return S_OK;}
};
using Guard=nativegloves::IndexGuard<Device,Buffer>;
static void GuardTests(){
    {Device d;{Guard g(&d,&d.replacement);assert(g.valid&&d.current==&d.replacement);}
        assert(d.current==&d.original&&d.original.refs==1&&d.calls==2);}
    {Device d;{Guard g(&d,nullptr);assert(!g.valid);}assert(!d.calls&&d.original.refs==1);}
    {Device d;d.getFail=true;{Guard g(&d,&d.replacement);assert(!g.valid);}assert(!d.calls&&d.original.refs==1);}
    for(unsigned i=0;i<3;++i){Device d;if(i==0)d.original.fail=true;
        if(i==1)d.original.desc.Size=4;if(i==2)d.original.desc.Format=D3DFMT_INDEX32;
        {Guard g(&d,&d.replacement);assert(!g.valid);}assert(!d.calls&&d.original.refs==1);}
    {Device d;d.setFail=true;{Guard g(&d,&d.replacement);assert(!g.valid);}assert(d.current==&d.original&&d.original.refs==1);}
    {Device d;Guard g(&d,&d.replacement);d.setFail=true;assert(FAILED(g.Restore()));assert(!g.valid);}
}
static void Identity(float palette[135][4]){
    std::memset(palette,0,135*4*sizeof(float));
    for(unsigned i=0;i<45;++i)for(unsigned a=0;a<3;++a)palette[i*3+a][a]=1;
}
static void QuaternionTests(){
    // Decode rejects malformed data transactionally, including any non-finger rotation.
    std::vector<unsigned char> p(32+6*2+90*16);std::memcpy(p.data(),"SVNI",4);
    const std::uint32_t header[]={1,6,2924,4830,16512,1,90};std::memcpy(p.data()+4,header,28);
    nativegloves::Data out;assert(!nativegloves::Decode(p.data(),p.size(),out));
}
int main(int argc,char** argv){
    GuardTests();QuaternionTests();
    for(unsigned b=0;b<90;++b){
        const bool th=nativegloves::Thumb(b);
        assert(th==((b>=10&&b<=12)||(b>=70&&b<=72)));
    }
    if(argc!=2){std::puts("PASS native index guard CPU fake; native asset checks not requested");return 0;}
    nativegloves::Renderer renderer;assert(renderer.Load(argv[1])&&renderer.Ready());
    std::vector<unsigned char> bytes;assert(gloves::Read(argv[1],bytes,128*1024));
    nativegloves::Data decoded;assert(nativegloves::Decode(bytes.data(),bytes.size(),decoded));
    for(unsigned offset: {0u,4u,8u,12u,16u,20u,28u}){
        auto bad=bytes;bad[offset]^=255;assert(!nativegloves::Decode(bad.data(),bad.size(),decoded));
    }
    {auto bad=bytes;std::uint32_t zero=0;std::memcpy(bad.data()+24,&zero,4);
        assert(!nativegloves::Decode(bad.data(),bad.size(),decoded));}
    {auto bad=bytes;bad[32]=bad[33]=255;assert(!nativegloves::Decode(bad.data(),bad.size(),decoded));}
    {auto bad=bytes;const float nan=NAN;std::memcpy(bad.data()+32+gloves::U32(bytes.data()+8)*2,&nan,4);
        assert(!nativegloves::Decode(bad.data(),bad.size(),decoded));}
    float palette[135][4],before[135][4],open[135][4];Identity(palette);
    // Separate wrist transforms: rotation + translation + .75 native scale.
    for(unsigned slot=0;slot<gloveisolate::kNoTmdCookedPaletteCount;++slot){
        const auto bone=gloveisolate::kNoTmdCookedPalette[slot];
        const int side=gloveisolate::NoTmdCookedHandSide(bone);
        for(unsigned row=0;row<3;++row){palette[slot*3+row][row]=.75f;palette[slot*3+row][3]=float(10+side*20+row);}
    }
    std::memcpy(before,palette,sizeof(before));
    const float release[]={0,0};assert(renderer.Pose(palette,release));std::memcpy(open,palette,sizeof(open));
    // Bind-open is a single rigid wrist delta, including forearm-weighted cuff.
    for(unsigned slot=0;slot<gloveisolate::kNoTmdCookedPaletteCount;++slot){
        const int side=gloveisolate::NoTmdCookedHandSide(gloveisolate::kNoTmdCookedPalette[slot]);
        for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)
            assert(std::fabs(palette[slot*3+r][c]-(c==3?float(10+side*20+r):(r==c?.75f:0)))<.0001f);
    }
    for(unsigned frame=0;frame<=100;++frame){
        std::memcpy(palette,before,sizeof(before));const float grips[]={float(frame)/100,1-float(frame)/100};
        assert(renderer.Pose(palette,grips));
        for(unsigned slot=0;slot<gloveisolate::kNoTmdCookedPaletteCount;++slot){
            const auto b=gloveisolate::kNoTmdCookedPalette[slot];
            for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c){float dot=0;
                for(unsigned k=0;k<3;++k)dot+=palette[slot*3+r][k]*palette[slot*3+c][k];
                assert(std::fabs(dot-(r==c?.75f*.75f:0))<.0001f);}
            if(!nativegloves::Finger(b))assert(std::memcmp(&palette[slot*3],&before[slot*3],3*4*sizeof(float))==0);
        }
        constexpr unsigned used=gloveisolate::kNoTmdCookedPaletteCount*3;
        assert(!std::memcmp(&palette[used],&before[used],(135-used)*4*sizeof(float)));
    }
    const float invalid[]={NAN,1};std::memcpy(palette,before,sizeof(before));
    assert(!renderer.Pose(palette,invalid)&&!std::memcmp(palette,before,sizeof(before)));
    const float leftOnly[]={1,0};assert(renderer.Pose(palette,leftOnly));
    bool leftMoved=false;
    for(unsigned slot=0;slot<gloveisolate::kNoTmdCookedPaletteCount;++slot){
        const auto b=gloveisolate::kNoTmdCookedPalette[slot];
        if(gloveisolate::NoTmdCookedHandSide(b)==1)
            for(unsigned f=0;f<12;++f)assert(std::fabs((&palette[slot*3][0])[f]-(&open[slot*3][0])[f])<.0001f);
        else if(nativegloves::Finger(b)&&std::memcmp(&palette[slot*3],&open[slot*3],3*4*sizeof(float)))leftMoved=true;
    }
    assert(leftMoved);std::memcpy(palette,before,sizeof(before));assert(renderer.Pose(palette,release));
    assert(!std::memcmp(palette,open,sizeof(open)));
    const float cylinderGrip[]={.45f,1.f};
    std::memcpy(palette,before,sizeof(before));
    assert(renderer.Pose(palette,cylinderGrip,nullptr,-1,1u,true));
    for(unsigned slot=0;slot<gloveisolate::kNoTmdCookedPaletteCount;++slot){
        const auto b=gloveisolate::kNoTmdCookedPalette[slot];
        if(gloveisolate::NoTmdCookedHandSide(b)==1)
            assert(!memcmp(palette+slot*3,before+slot*3,3*4*sizeof(float)));
    }
    const auto start=std::chrono::steady_clock::now();
    for(unsigned i=0;i<3600;++i){std::memcpy(palette,before,sizeof(before));assert(renderer.Pose(palette,leftOnly));}
    const auto ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/3600;
    std::printf("PASS native original-rig pose 101 analog levels, rigid .75 scale, wrist/cuff, per-hand release, untouched registers; CPU %.4f ms/pose; triangles=%u cap=%u\n",ms,renderer.Triangles(),renderer.Caps());
}
