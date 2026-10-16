#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d9.h>
#include "../native_glove_cuffs.h"
#include <cassert>
#include <cstdio>

static void U32(std::vector<unsigned char>& b,unsigned n){const auto p=(const unsigned char*)&n;b.insert(b.end(),p,p+4);}
static std::vector<unsigned char> Fixture(){
    std::vector<unsigned char> b{'S','V','N','C'};U32(b,1);U32(b,2);U32(b,0);
    for(unsigned i=0;i<2;++i){
        U32(b,i+1);U32(b,i?2626:2924);U32(b,i?4402:4830);U32(b,i?36369:16512);U32(b,12);U32(b,2);
        const std::uint16_t indices[]={0,1,2,0,2,3,0,3,4,0,4,1};
        const auto p=(const unsigned char*)indices;b.insert(b.end(),p,p+sizeof(indices));
    }return b;
}
struct Buffer {
    unsigned refs=1;D3DINDEXBUFFER_DESC desc{};
    Buffer(){desc.Format=D3DFMT_INDEX16;desc.Size=33024;}
    HRESULT GetDesc(D3DINDEXBUFFER_DESC* p){*p=desc;return S_OK;}
    void Release(){assert(refs>1);--refs;}
};
struct Dev {
    Buffer original,replacement;Buffer* bound=&original;unsigned calls=0;
    HRESULT GetIndices(Buffer** b){*b=bound;++bound->refs;return S_OK;}
    HRESULT SetIndices(Buffer* b){bound=b;++calls;return S_OK;}
};
int main(int argc,char** argv){
    for(unsigned bits=0;bits<(1u<<11);++bits){
        const auto b=[&](unsigned n){return (bits&(1u<<n))!=0;};
        const bool expected=b(0)&&b(1)&&b(2)&&b(3)&&b(4)&&b(5)&&b(6)&&!b(7)&&!b(8)&&!b(9)&&!b(10);
        assert(glovecuffs::Eligible(b(0),b(1),b(2),b(3),2,b(4),b(5),b(6),b(7),b(8),b(9),b(10))==expected);
    }
    for(int isolate:{0,1,3})assert(!glovecuffs::Eligible(true,true,true,true,isolate,true,true,true,false,false,false,false));
    assert(glovecuffs::Profile(3314,0,4830)==0&&glovecuffs::Profile(2924,0,4830)==0);
    assert(glovecuffs::Profile(7444,0,4402)==1&&glovecuffs::Profile(2626,0,4402)==1);
    assert(glovecuffs::Profile(5799,0,4830)==-1&&glovecuffs::Profile(3314,3,4830)==-1&&glovecuffs::Profile(3314,0,4831)==-1);
    auto bytes=Fixture();glovecuffs::Data data;assert(glovecuffs::Decode(bytes.data(),bytes.size(),data));
    for(std::size_t size=0;size<bytes.size();++size)assert(!glovecuffs::Decode(bytes.data(),size,data));
    for(unsigned offset:{0u,4u,8u,12u,16u,20u,24u,28u,32u,36u,64u,68u,72u,76u,80u,84u}){
        auto bad=bytes;bad[offset]^=0xff;assert(!glovecuffs::Decode(bad.data(),bad.size(),data));
    }
    {auto bad=bytes;bad.push_back(0);assert(!glovecuffs::Decode(bad.data(),bad.size(),data));}
    for(unsigned offset:{40u,42u,44u,88u,90u,92u}){auto bad=bytes;bad[offset]=bad[offset+1]=255;
        assert(!glovecuffs::Decode(bad.data(),bad.size(),data));}
    {Dev dev;{nativegloves::IndexGuard<Dev,Buffer> guard(&dev,&dev.replacement,72738);assert(!guard.valid);}
        assert(!dev.calls&&dev.original.refs==1);}
    {Dev dev;dev.original.desc.Size=72738;{nativegloves::IndexGuard<Dev,Buffer> guard(&dev,&dev.replacement,72738);assert(guard.valid);}
        assert(dev.bound==&dev.original&&dev.calls==2&&dev.original.refs==1);}
    if(argc==2){
        std::vector<unsigned char> asset;assert(gloves::Read(argv[1],asset,128*1024));
        assert(glovecuffs::Decode(asset.data(),asset.size(),data));
        assert(data.mesh[0].indices.size()==4442*3&&data.mesh[0].caps==48);
        assert(data.mesh[1].indices.size()==3994*3&&data.mesh[1].caps==28);
    }
    std::puts("PASS cuff signatures/gates, bounded decoder, transactional rejection, source IB size/refcounts (CPU only)");
}
