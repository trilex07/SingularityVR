#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cassert>
#include <cstdio>
#include "../knife_renderer.h"
#include "../glove_native.h"

static void Identity(float p[135][4]){
    std::memset(p,0,135*4*sizeof(float));for(unsigned s=0;s<45;++s)for(unsigned c=0;c<3;++c)p[s*3+c][c]=1;
}
static void CompileShaders(){
    HMODULE m=LoadLibraryExW(L"d3dcompiler_47.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);assert(m);
    using Fn=HRESULT(WINAPI*)(LPCVOID,SIZE_T,LPCSTR,const D3D_SHADER_MACRO*,ID3DInclude*,LPCSTR,LPCSTR,UINT,UINT,ID3DBlob**,ID3DBlob**);
    auto compile=reinterpret_cast<Fn>(GetProcAddress(m,"D3DCompile"));assert(compile);
    for(unsigned stage=0;stage<2;++stage){ID3DBlob *code=nullptr,*err=nullptr;
        HRESULT hr=compile(knifelight::shader,sizeof(knifelight::shader)-1,"knife-lighting-L1",nullptr,nullptr,
            stage?"psMain":"vsMain",stage?"ps_3_0":"vs_3_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&code,&err);
        if(FAILED(hr)&&err){std::fprintf(stderr,"%s\n",static_cast<const char*>(err->GetBufferPointer()));std::fflush(stderr);}
        assert(SUCCEEDED(hr)&&code);std::printf("PASS compiled %s SM3 bytecode %u bytes (CPU, not GPU execution)\n",stage?"PS":"VS",(unsigned)code->GetBufferSize());
        hipknife::Release(code);hipknife::Release(err);
    }
    FreeLibrary(m);
}
static void ThumbDecoder(){
    std::vector<unsigned char> p(312);std::memcpy(p.data(),"SVNT",4);unsigned header[3]={3,2,3};std::memcpy(p.data()+4,header,12);
    const float one=1;std::memcpy(p.data()+20,&one,4);
    for(unsigned i=0;i<18;++i)std::memcpy(p.data()+24+i*16+12,&one,4);
    thumbpose::Data out;assert(thumbpose::Decode(p.data(),p.size(),out));const auto good=out;
    for(unsigned offset:{0u,4u,8u,12u,16u,28u}){auto bad=p;const float nan=NAN;std::memcpy(bad.data()+offset,&nan,4);
        assert(!thumbpose::Decode(bad.data(),bad.size(),out));assert(!std::memcmp(&out,&good,sizeof(out)));}
    assert(!thumbpose::Decode(p.data(),311,out));
    {auto bad=p;const float zero=0;std::memcpy(bad.data()+20,&zero,4);assert(!thumbpose::Decode(bad.data(),bad.size(),out));}
}
int main(int argc,char** argv){
    CompileShaders();ThumbDecoder();
    const float depth[4]={-.01f,1.01f,0,0};assert(std::fabs(knifelight::DepthAlpha(1,depth)-1)<1e-6f);
    assert(std::fabs(knifelight::DepthAlpha(.1f,depth)-.91f)<1e-6f);
    if(argc<6){std::puts("PASS synthetic thumb decoder/depth alpha; private replay not requested");return 0;}
    std::vector<unsigned char> bytes;assert(gloves::Read(argv[1],bytes,8192));thumbpose::Data thumb;assert(thumbpose::Decode(bytes.data(),bytes.size(),thumb));
    nativegloves::Renderer native;assert(native.Load(argv[2])&&native.LoadThumbs(argv[1]));
    assert(gloves::Read(argv[3],bytes,1024));knifehand::Data grip;assert(knifehand::Decode(bytes.data(),bytes.size(),grip));
    std::vector<unsigned char> golden;assert(gloves::Read(argv[4],golden,2*1024*1024));
    assert(golden.size()>=8&&!std::memcmp(golden.data(),"SVTG",4)&&gloves::U32(golden.data()+4)==12);size_t at=8;unsigned total=0;
    for(unsigned group=0;group<12;++group){assert(at+12<=golden.size());const auto mode=gloves::U32(golden.data()+at),side=gloves::U32(golden.data()+at+4),n=gloves::U32(golden.data()+at+8);at+=12;
        assert(mode<=5&&side<=1&&n<2924&&at+n*56<=golden.size());
        float p[135][4],freeGrip[2]={},digits[2][5]{};Identity(p);
        if(mode==5)std::memcpy(digits[side],grip.fingers[side],20);else freeGrip[side]=(mode+1)*.2f;
        assert(native.Pose(p,freeGrip,mode==5?digits:nullptr,mode==5?(int)side:-1));
        for(unsigned i=0;i<n;++i,at+=56){const auto* b=golden.data()+at;float xyz[3],w[4],expected[3];unsigned bones[4];
            std::memcpy(xyz,b,12);std::memcpy(w,b+12,16);std::memcpy(bones,b+28,16);std::memcpy(expected,b+44,12);float actual[3]{};
            for(unsigned j=0;j<4;++j)if(w[j]>0){unsigned slot=999;
                for(unsigned s=0;s<gloveisolate::kNoTmdCookedPaletteCount;++s)if(gloveisolate::kNoTmdCookedPalette[s]==bones[j])slot=s;
                assert(slot!=999);for(unsigned c=0;c<3;++c){const auto* m=p[slot*3+c];actual[c]+=w[j]*(xyz[0]*m[0]+xyz[1]*m[1]+xyz[2]*m[2]+m[3]);}}
            for(unsigned c=0;c<3;++c)assert(std::fabs(actual[c]-expected[c])<.00015f);++total;
        }
    }
    assert(at==golden.size());
    // Continuous analog + opposite hand, cuff and non-thumb invariance versus
    // accepted original rig. No geometry/mount/proportion change is allowed.
    nativegloves::Renderer baseline;assert(baseline.Load(argv[2]));
    for(unsigned step=0;step<=100;++step){const float g[2]={step*.01f,0};float a[135][4],b[135][4];Identity(a);Identity(b);
        assert(native.Pose(a,g)&&baseline.Pose(b,g));
        for(unsigned s=0;s<gloveisolate::kNoTmdCookedPaletteCount;++s){unsigned bone=gloveisolate::kNoTmdCookedPalette[s];
            if(!nativegloves::Thumb(bone)||bone>=70)assert(!std::memcmp(a+s*3,b+s*3,12*sizeof(float)));
            for(unsigned c=0;c<3;++c)for(unsigned r=0;r<4;++r)assert(std::isfinite(a[s*3+c][r]));}
    }
    assert(gloves::Read(argv[5],bytes,1024*1024));std::vector<knifelight::Vertex> v;std::vector<WORD> ix;
    assert(knifelight::Decode(bytes.data(),bytes.size(),v,ix)&&v.size()==662&&ix.size()==3030);
    {auto bad=bytes;float nan=NAN;std::memcpy(bad.data()+16+12,&nan,4);assert(!knifelight::Decode(bad.data(),bad.size(),v,ix));}
    assert(!knifelight::Decode(bytes.data(),bytes.size()-1,v,ix));
    {std::string path(argv[5]);path.resize(path.find_last_of("\\/")+1);knifelight::Renderer lit;
        assert(lit.Load(path));lit.Release();assert(lit.Load(path));lit.Release();}
    if(argc>=8){for(unsigned stage=0;stage<2;++stage){assert(gloves::Read(argv[6+stage],bytes,16384));
        assert(knifelight::Hash(bytes.data(),(unsigned)bytes.size())==(stage?0x85D66574CA9E9C00ull:0x37754B7CC68D8D59ull));}}
    std::printf("PASS %u native thumb golden vertices, 12 mode/hand profiles, 101 analog levels, tangent frames, exact shader identities\n",total);
}
