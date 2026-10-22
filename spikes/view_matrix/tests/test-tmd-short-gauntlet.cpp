#define NOMINMAX
#include "../tmd_short_gauntlet.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <vector>

struct Buffer {
    unsigned refs=1;D3DINDEXBUFFER_DESC desc{};
    Buffer(){desc.Format=D3DFMT_INDEX16;desc.Size=36369*2;}
    HRESULT GetDesc(D3DINDEXBUFFER_DESC* p){*p=desc;return S_OK;}
    void Release(){assert(refs>1);--refs;}
};
struct Dev {
    Buffer original,replacement;Buffer* bound=&original;bool failBind=false,failRestore=false;
    unsigned binds=0,restores=0;
    HRESULT GetIndices(Buffer** b){*b=bound;++bound->refs;return S_OK;}
    HRESULT SetIndices(Buffer* b){
        if(b==&replacement){++binds;if(failBind)return E_FAIL;}
        if(b==&original){++restores;if(failRestore)return E_FAIL;}
        bound=b;return S_OK;
    }
};

static void TestDrawRoute() {
    // Same dispatch function called by the StereoPair lambda. The shorter
    // generic glove count must never win when SVTG is successfully bound.
    {Dev dev;unsigned shortCalls=0,nativeCalls=0;
        nativegloves::IndexGuard<Dev,Buffer> guard(&dev,&dev.replacement,36369*2);
        assert(guard.valid&&dev.bound==&dev.replacement);
        for(unsigned eye=0;eye<2;++eye){
            const auto r=tmdgauntlet::DispatchBound(true,guard.valid,7110,3994,
                [&](unsigned start,unsigned triangles){assert(start==0&&triangles==7110);++shortCalls;return S_OK;},
                [&](unsigned,unsigned){++nativeCalls;return S_OK;});
            assert(r.handled&&r.shortTmd&&SUCCEEDED(r.hr));
        }
        const HRESULT restored=guard.Restore();
        assert(tmdgauntlet::SuccessfulStereoBoundDraw(true,shortCalls,S_OK,restored));
        assert(shortCalls==2&&nativeCalls==0&&dev.bound==&dev.original&&dev.restores==1);
    }
    // Flag OFF: existing native glove/cuff route and triangle count remain.
    {Dev dev;unsigned shortCalls=0,nativeCalls=0;
        nativegloves::IndexGuard<Dev,Buffer> guard(&dev,&dev.replacement,36369*2);
        const auto r=tmdgauntlet::DispatchBound(false,guard.valid,7110,3994,
            [&](unsigned,unsigned){++shortCalls;return S_OK;},
            [&](unsigned start,unsigned triangles){assert(start==0&&triangles==3994);++nativeCalls;return S_OK;});
        assert(r.handled&&!r.shortTmd&&shortCalls==0&&nativeCalls==1);
        assert(SUCCEEDED(guard.Restore())&&dev.bound==&dev.original);
    }
    // Missing/corrupt cache or failed GPU buffer creation/binding: the exact
    // 49-run original hardware renderer is called, never 7110 or glove count.
    for(unsigned failure=0;failure<3;++failure){
        Dev dev;dev.failBind=failure==2;
        Buffer* replacement=failure==0?nullptr:&dev.replacement;
        if(failure==1)replacement=nullptr; // decoder rejected corrupt SVTG
        nativegloves::IndexGuard<Dev,Buffer> guard(&dev,replacement,36369*2);
        assert(!guard.valid&&dev.bound==&dev.original);
        const auto r=tmdgauntlet::DispatchBound(false,guard.valid,7110,3994,
            [&](unsigned,unsigned){assert(false);return E_FAIL;},
            [&](unsigned,unsigned){assert(false);return E_FAIL;});
        assert(!r.handled);
        unsigned runs=0,faces=0;
        const HRESULT h=tmdgauntlet::DrawHardwareFallback(13206,[&](unsigned first,unsigned count){
            assert(first>=13206&&count>0);++runs;faces+=count;return S_OK;});
        assert(SUCCEEDED(h)&&runs==49&&faces==6527);
        assert(!tmdgauntlet::SuccessfulStereoBoundDraw(false,0,h,S_OK));
    }
    // Even after a successful draw, a failed restore must not authorize tail
    // suppression. A different Present token (Reset/new frame) expires it.
    {Dev dev;dev.failRestore=true;
        nativegloves::IndexGuard<Dev,Buffer> guard(&dev,&dev.replacement,36369*2);
        assert(guard.valid);
        const auto r=tmdgauntlet::DispatchBound(true,guard.valid,7110,3994,
            [&](unsigned,unsigned){return S_OK;},[&](unsigned,unsigned){assert(false);return E_FAIL;});
        assert(r.handled&&r.shortTmd&&FAILED(guard.Restore()));
        assert(!tmdgauntlet::SuccessfulStereoBoundDraw(true,2,r.hr,E_FAIL));
    }
    assert(tmdgauntlet::TailAfterSuccessfulBase(7,101,7,100));
    assert(!tmdgauntlet::TailAfterSuccessfulBase(8,101,7,100));
    assert(!tmdgauntlet::TailAfterSuccessfulBase(7,110,7,100));
    assert(!tmdgauntlet::TailAfterSuccessfulBase(7,101,-1,100));
}

int main(int argc,char** argv) {
    TestDrawRoute();
    if(argc>1){
        tmdgauntlet::Renderer renderer;
        assert(renderer.Load(argv[1]));
        assert(renderer.Ready()&&renderer.Triangles()==7110);
        renderer.ReleaseGPU(); // Reset drops GPU state, not validated CPU geometry.
        assert(renderer.Ready()&&renderer.Triangles()==7110);
    }
    std::vector<unsigned char> bytes(24 + 7110 * 6);
    auto put=[](unsigned char* p,unsigned x){std::memcpy(p,&x,4);};
    std::memcpy(bytes.data(),"SVTG",4);
    put(bytes.data()+4,1);put(bytes.data()+8,7444);put(bytes.data()+12,7110);
    put(bytes.data()+16,23);put(bytes.data()+20,36369);
    for (unsigned i=0;i<7110;++i) {
        const std::uint16_t tri[3]={0,1,2};
        std::memcpy(bytes.data()+24+i*6,tri,6);
    }
    tmdgauntlet::Data data;
    assert(tmdgauntlet::Decode(bytes.data(),bytes.size(),data));
    assert(data.indices.size()==7110*3 && data.capTriangles==23);
    assert(!tmdgauntlet::Decode(bytes.data(),bytes.size()-1,data));
    bytes[4]=2;assert(!tmdgauntlet::Decode(bytes.data(),bytes.size(),data));bytes[4]=1;
    std::uint16_t bad=7444;std::memcpy(bytes.data()+24,&bad,2);
    assert(!tmdgauntlet::Decode(bytes.data(),bytes.size(),data));
    assert(tmdgauntlet::Eligible(true,true,true,true,2,true,false,false));
    assert(!tmdgauntlet::Eligible(false,true,true,true,2,true,false,false));
    assert(!tmdgauntlet::Eligible(true,true,false,true,2,true,false,false));
    assert(!tmdgauntlet::Eligible(true,true,true,true,2,true,true,false));
    assert(!tmdgauntlet::Eligible(true,true,true,true,2,true,false,true));
    assert(tmdgauntlet::IsRightForearmTail(7444,36318,17));
    assert(!tmdgauntlet::IsRightForearmTail(7444,13206,7704));
    std::puts("PASS: actual bound dispatch, 7110 stereo draws, 49-run fallback, restore/tail receipt");
    return 0;
}
