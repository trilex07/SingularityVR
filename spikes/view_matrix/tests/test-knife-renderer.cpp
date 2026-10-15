#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "../knife_renderer.h"
#include <cassert>
#include <chrono>

// CPU-only D3D surface/state fake: no real device, GPU, game, or Steam.
struct Surface {unsigned refs=1;void AddRef(){++refs;}void Release(){assert(refs>1);--refs;}};
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
int main(int argc,char** argv){
    const Calls calls;
    {Device d;Block b{&d};{Guard guard(&d,&b,calls);assert(guard.valid);Mutate(d);}Restored(d,b);}
    {Device d;Block b{&d};assert(!EarlyReturn(d,b,calls));Restored(d,b);}
    for(int fail=0;fail<4;++fail){Device d;d.failRT=fail;Block b{&d};
        {Guard guard(&d,&b,calls);assert(!guard.valid);}
        assert(!d.changes&&!b.applied&&d.a.refs==1&&d.b.refs==1&&d.c.refs==1);}
    {Device d;Block b{&d};b.fail=true;{Guard guard(&d,&b,calls);assert(!guard.valid);}assert(!d.changes&&!b.applied);}
    // Cache decoder: fail closed on sizes, version, indices and nonfinite mesh.
    std::vector<hipknife::Vertex> v;std::vector<WORD> ix;
    std::vector<unsigned char> bytes(16+3*sizeof(hipknife::Vertex)+6);
    std::memcpy(bytes.data(),"SVRK",4);unsigned header[3]={1,3,3};
    std::memcpy(bytes.data()+4,header,12);
    const hipknife::Vertex verts[3]={{0,0,0,~0u},{0,.1f,0,~0u},{.1f,0,0,~0u}};
    const WORD indices[3]={0,1,2};
    std::memcpy(bytes.data()+16,verts,sizeof(verts));std::memcpy(bytes.data()+16+sizeof(verts),indices,6);
    assert(hipknife::Decode(bytes.data(),bytes.size(),v,ix));assert(v.size()==3&&ix.size()==3);
    assert(!hipknife::Decode(bytes.data(),bytes.size()-1,v,ix));
    bytes[4]=2;assert(!hipknife::Decode(bytes.data(),bytes.size(),v,ix));bytes[4]=1;
    bytes[bytes.size()-2]=3;assert(!hipknife::Decode(bytes.data(),bytes.size(),v,ix));
    if(argc==2){hipknife::Renderer r;const auto start=std::chrono::steady_clock::now();assert(r.Load(argv[1]));
        std::printf("CPU eager knife load %.3f ms, %u verts / %u tris\n",
            std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count(),r.Vertices(),r.Triangles());}
    std::puts("PASS knife CPU fake: decoder, RT/MRT/DS/viewport/state/refcounts + early return/capture failures");
}
