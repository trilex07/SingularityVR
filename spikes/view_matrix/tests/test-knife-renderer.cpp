#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "../knife_renderer.h"
#include <cassert>
#include <chrono>

// CPU-only D3D surface/state fake: no real device, GPU, game, or Steam.
struct Surface {unsigned refs=1,clears=0;void AddRef(){++refs;}void Release(){assert(refs>1);--refs;}};
struct Device {
    Surface a,b,c,depth,replacement;
    Surface* rt[4]={&a,&b,&c,nullptr};Surface* ds=&depth;
    D3DVIEWPORT9 vp{12,24,800,600,0,1};int state=71,failRT=-1;unsigned changes=0;
    HRESULT GetDeviceCaps(D3DCAPS9* p){*p={};p->NumSimultaneousRTs=4;return S_OK;}
    HRESULT GetViewport(D3DVIEWPORT9* p){*p=vp;return S_OK;}
    HRESULT GetRenderTarget(DWORD i,Surface** p){if(int(i)==failRT)return E_FAIL;*p=rt[i];if(*p)(*p)->AddRef();return *p?S_OK:D3DERR_NOTFOUND;}
    HRESULT GetDepthStencilSurface(Surface** p){*p=ds;if(*p)(*p)->AddRef();return *p?S_OK:D3DERR_NOTFOUND;}
    HRESULT SetDepthStencilSurface(Surface* p){ds=p;++changes;return S_OK;}
};
struct Block {
    Device* dev;int saved=0;bool fail=false;unsigned applied=0;
    HRESULT Capture(){if(fail)return E_FAIL;saved=dev->state;return S_OK;}
    HRESULT Apply(){dev->state=saved;++applied;return S_OK;}
};
struct Calls {
    static HRESULT target(Device* d,DWORD i,Surface* p){d->rt[i]=p;++d->changes;return S_OK;}
    static HRESULT viewport(Device* d,const D3DVIEWPORT9* p){d->vp=*p;++d->changes;return S_OK;}
    static HRESULT clear(Device* d,DWORD,const D3DRECT*,DWORD flags,D3DCOLOR,float depth,DWORD stencil){
        assert(flags==D3DCLEAR_ZBUFFER&&depth==1&&!stencil&&d->ds);++d->ds->clears;return S_OK;}
};
using Guard=hipknife::StateGuard<Device,Block,Surface,Calls>;
static void Mutate(Device& d){d.rt[0]=&d.replacement;d.rt[1]=d.rt[2]=nullptr;d.ds=nullptr;d.state=-99;d.vp={0,0,320,200,0,1};}
static void Restored(const Device& d,const Block& b){
    assert(d.rt[0]==&d.a&&d.rt[1]==&d.b&&d.rt[2]==&d.c&&!d.rt[3]);
    assert(d.ds==&d.depth&&d.state==71&&b.applied==1);
    assert(d.vp.X==12&&d.vp.Y==24&&d.vp.Width==800&&d.vp.Height==600);
    assert(d.a.refs==1&&d.b.refs==1&&d.c.refs==1&&d.depth.refs==1);
}
static bool EarlyReturn(Device& d,Block& b,const Calls& c){Guard guard(&d,&b,c);assert(guard.valid);Mutate(d);return false;}
struct Texture {
    D3DPOOL pool=D3DPOOL_DEFAULT;unsigned refs=0,locks=0,unlocks=0;
    bool lockFail=false,unlockFail=false;int pitch=12;
    unsigned char data[24]{};
    void Release(){assert(refs);--refs;}
    HRESULT LockRect(UINT,D3DLOCKED_RECT* r,const RECT*,DWORD){
        assert(pool==D3DPOOL_SYSTEMMEM);++locks;
        if(lockFail)return E_FAIL;r->Pitch=pitch;r->pBits=data;return S_OK;}
    HRESULT UnlockRect(UINT){++unlocks;return unlockFail?E_FAIL:S_OK;}
};
struct UploadDevice {
    Texture staging,gpu;unsigned creates=0,updates=0;int createFail=-1;bool updateFail=false;
    HRESULT CreateTexture(UINT w,UINT h,UINT levels,DWORD usage,D3DFORMAT format,D3DPOOL pool,Texture** p,HANDLE*){
        assert(w==2&&h==2&&levels==1&&!usage&&format==D3DFMT_A8R8G8B8);
        const unsigned call=creates++;if(int(call)==createFail)return E_FAIL;
        assert(pool==(call?D3DPOOL_DEFAULT:D3DPOOL_SYSTEMMEM));
        *p=call?&gpu:&staging;(*p)->pool=pool;(*p)->refs=1;return S_OK;}
    HRESULT UpdateTexture(Texture* from,Texture* to){
        assert(from==&staging&&to==&gpu&&!gpu.locks);++updates;
        return updateFail?E_FAIL:S_OK;}
};
static void UploadTests(){
    const std::vector<unsigned char> pixels={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    {UploadDevice d;Texture* gpu=nullptr;assert(hipknife::UploadAlbedo(&d,2,2,pixels,gpu));
        assert(gpu==&d.gpu&&!d.staging.refs&&d.gpu.refs==1&&d.staging.locks==1&&d.staging.unlocks==1);
        assert(!std::memcmp(d.staging.data,pixels.data(),8)&&!std::memcmp(d.staging.data+12,pixels.data()+8,8));
        assert(!d.staging.data[8]&&!d.staging.data[9]);hipknife::Release(gpu);assert(!gpu&&!d.gpu.refs);}
    for(unsigned failure=0;failure<6;++failure){UploadDevice d;Texture* gpu=nullptr;
        if(failure<2)d.createFail=(int)failure;
        if(failure==2)d.staging.lockFail=true;if(failure==3)d.staging.unlockFail=true;
        if(failure==4)d.staging.pitch=1;if(failure==5)d.updateFail=true;
        assert(!hipknife::UploadAlbedo(&d,2,2,pixels,gpu));
        assert(!gpu&&!d.staging.refs&&!d.gpu.refs&&!d.gpu.locks);
    }
}
int main(int argc,char** argv){
    UploadTests();
    const Calls calls;
    {Device d;Block b{&d};{Guard guard(&d,&b,calls);assert(guard.valid);
        assert(!hipknife::PrimePrivateDepth(&d,&d.depth,&d.depth,calls)&&!d.depth.clears);
        assert(hipknife::PrimePrivateDepth(&d,&d.replacement,&d.depth,calls));}
        Restored(d,b);assert(!d.depth.clears&&d.replacement.clears==1);}
    {Device d;Block b{&d};{Guard guard(&d,&b,calls);assert(guard.valid);Mutate(d);}Restored(d,b);}
    {Device d;Block b{&d};assert(!EarlyReturn(d,b,calls));Restored(d,b);}
    for(int fail=0;fail<4;++fail){Device d;d.failRT=fail;Block b{&d};
        {Guard guard(&d,&b,calls);assert(!guard.valid);}
        assert(!d.changes&&!b.applied&&d.a.refs==1&&d.b.refs==1&&d.c.refs==1);}
    {Device d;Block b{&d};b.fail=true;{Guard guard(&d,&b,calls);assert(!guard.valid);}assert(!d.changes&&!b.applied);}
    // Cache decoder: fail closed on sizes, version, indices and nonfinite mesh.
    std::vector<hipknife::Vertex> v;std::vector<WORD> ix;
    std::vector<unsigned char> bytes(16+3*sizeof(hipknife::Vertex)+6);
    std::memcpy(bytes.data(),"SVRK",4);unsigned header[3]={2,3,3};
    std::memcpy(bytes.data()+4,header,12);
    const hipknife::Vertex verts[3]={{0,0,0,~0u,0,0},{0,.1f,0,~0u,1,0},{.1f,0,0,~0u,.25f,.75f}};
    const WORD indices[3]={0,1,2};
    std::memcpy(bytes.data()+16,verts,sizeof(verts));std::memcpy(bytes.data()+16+sizeof(verts),indices,6);
    assert(hipknife::Decode(bytes.data(),bytes.size(),v,ix));assert(v.size()==3&&ix.size()==3);
    assert(!hipknife::Decode(bytes.data(),bytes.size()-1,v,ix));
    assert(v[2].u==.25f&&v[2].v==.75f&&v[0].colour==0xffffffff);
    {auto bad=bytes;const float nan=NAN;std::memcpy(bad.data()+16+16,&nan,4);
        assert(!hipknife::Decode(bad.data(),bad.size(),v,ix));}
    {auto bad=bytes;bad[16+12]=0;assert(!hipknife::Decode(bad.data(),bad.size(),v,ix));}
    bytes[4]=1;assert(!hipknife::Decode(bytes.data(),bytes.size(),v,ix));bytes[4]=2;
    bytes[bytes.size()-2]=3;assert(!hipknife::Decode(bytes.data(),bytes.size(),v,ix));
    // Texture cache bounds, BGRA order and alpha are preserved (alpha is not opacity).
    UINT tw=0,th=0;std::vector<unsigned char> texels;
    std::vector<unsigned char> image(20);std::memcpy(image.data(),"SVRT",4);
    const unsigned texHeader[3]={1,1,1};std::memcpy(image.data()+4,texHeader,12);
    image[16]=31;image[17]=47;image[18]=61;image[19]=0;
    assert(hipknife::DecodeTexture(image.data(),image.size(),tw,th,texels)&&tw==1&&th==1);
    assert(texels[0]==31&&texels[1]==47&&texels[2]==61&&texels[3]==0);
    assert(!hipknife::DecodeTexture(image.data(),image.size()-1,tw,th,texels));
    image[8]=0;assert(!hipknife::DecodeTexture(image.data(),image.size(),tw,th,texels));
    if(argc==2){hipknife::Renderer r;const auto start=std::chrono::steady_clock::now();assert(r.Load(argv[1]));
        std::printf("CPU eager knife + original diffuse load %.3f ms, %u verts / %u tris / %ux%u texture\n",
            std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count(),r.Vertices(),r.Triangles(),r.TextureWidth(),r.TextureHeight());
        assert(!r.Load("missing-private-knife.svrk")&&!r.Vertices()&&!r.TextureWidth());}
    if(argc==3){hipknife::Renderer r;const auto start=std::chrono::steady_clock::now();
        assert(r.LoadWithAlbedo(argv[1],argv[2]));
        std::printf("CPU eager explicit asset pair %.3f ms, %u verts / %u tris / %ux%u texture\n",
            std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count(),r.Vertices(),r.Triangles(),r.TextureWidth(),r.TextureHeight());
        assert(!r.LoadWithAlbedo(argv[1],"missing-private-albedo.svrt")&&!r.Vertices()&&!r.TextureWidth());}
    std::puts("PASS knife CPU fake: decoder, RT/MRT/DS/viewport/state/refcounts + early return/capture failures");
}
