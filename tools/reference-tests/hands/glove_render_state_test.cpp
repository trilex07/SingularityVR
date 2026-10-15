#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d9.h>
#include <cassert>
#include <cstdio>
#include <chrono>
#include "../../../spikes/view_matrix/glove_renderer.h"

// CPU fake of only the guard's API surface; no device creation, GPU or Steam.
// Proves guard sequencing/refcounts, not real driver state-block behavior.
struct Surface {unsigned refs=1;void AddRef(){++refs;}void Release(){assert(refs>1);--refs;}};
struct Device {
    Surface a,b,c,depth,replacement;
    Surface* rt[4]={&a,&b,&c,nullptr};Surface* ds=&depth;
    D3DVIEWPORT9 vp{12,24,800,600,0,1};int state=71,failGetRT=-1;unsigned changes=0;
    HRESULT GetDeviceCaps(D3DCAPS9* cOut){*cOut={};cOut->NumSimultaneousRTs=4;return S_OK;}
    HRESULT GetViewport(D3DVIEWPORT9* p){*p=vp;return S_OK;}
    HRESULT GetRenderTarget(DWORD i,Surface** p){if(int(i)==failGetRT)return E_FAIL;*p=rt[i];if(*p)(*p)->AddRef();return *p?S_OK:D3DERR_NOTFOUND;}
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
using Guard=gloves::StateGuard<Device,Block,Surface,Calls>;
static void Mutate(Device& d){
    d.rt[0]=&d.replacement;d.rt[1]=d.rt[2]=nullptr;d.ds=nullptr;
    d.state=-99;d.vp={0,0,320,200,0,1};
}
static void AssertRestored(const Device& d,const Block& b){
    assert(d.rt[0]==&d.a&&d.rt[1]==&d.b&&d.rt[2]==&d.c&&!d.rt[3]);
    assert(d.ds==&d.depth&&d.state==71&&b.applied==1);
    assert(d.vp.X==12&&d.vp.Y==24&&d.vp.Width==800&&d.vp.Height==600);
    assert(d.a.refs==1&&d.b.refs==1&&d.c.refs==1&&d.depth.refs==1);
}
static bool EarlyReturn(Device& d,Block& b,const Calls& c){Guard guard(&d,&b,c);assert(guard.valid);Mutate(d);return false;}
int main(int argc,char** argv){
    const Calls calls;
    {Device d;Block b{&d};{Guard guard(&d,&b,calls);assert(guard.valid);Mutate(d);}AssertRestored(d,b);}
    {Device d;Block b{&d};assert(!EarlyReturn(d,b,calls));AssertRestored(d,b);}
    for(int fail=0;fail<4;++fail){
        Device d;d.failGetRT=fail;Block b{&d};
        {Guard guard(&d,&b,calls);assert(!guard.valid);}
        assert(!d.changes&&!b.applied&&d.a.refs==1&&d.b.refs==1&&d.c.refs==1);
    }
    {Device d;Block b{&d};b.fail=true;{Guard guard(&d,&b,calls);assert(!guard.valid);}assert(!d.changes&&!b.applied);}
    if(argc==2){
        gloves::Renderer renderer;const auto start=std::chrono::steady_clock::now();
        assert(renderer.Load(argv[1]));
        const auto ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
        assert(!renderer.Ready()&&renderer.Vertices(0)>0&&renderer.Vertices(1)>0);
        std::printf("CPU eager load both caches + texture %.3f ms (no D3D device), vertices=%u/%u\n",ms,renderer.Vertices(0),renderer.Vertices(1));
        assert(!renderer.Load("nonexistent-private-glove-pack"));
        assert(!renderer.Ready());
    }
    std::puts("PASS glove CPU fake: RT/MRT/DS/viewport/state/refcounts, success + early return + capture failures");
}
