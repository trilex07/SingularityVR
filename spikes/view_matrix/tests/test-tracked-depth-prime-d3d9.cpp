#define NOMINMAX
#include "../tracked_depth_prime.h"
#include <cassert>
#include <cstdio>

// Real D3D9/Ex pixels through the production cache/dispatch predicate. Synthetic
// scene reproduces early depth-only old pose -> world -> late new pose. It does
// not establish the runtime identity of the user's black silhouette.
struct V {float x,y,z;DWORD color;};
static void Run(bool ex) {
    HWND w=CreateWindowA("STATIC","depth-prime offline",WS_OVERLAPPED,0,0,128,64,
        nullptr,nullptr,GetModuleHandle(nullptr),nullptr);assert(w);
    IDirect3D9 *api=nullptr;IDirect3D9Ex* apiEx=nullptr;
    if(ex){assert(SUCCEEDED(Direct3DCreate9Ex(D3D_SDK_VERSION,&apiEx)));api=apiEx;}
    else api=Direct3DCreate9(D3D_SDK_VERSION);
    assert(api);D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;
    pp.hDeviceWindow=w;pp.BackBufferWidth=128;pp.BackBufferHeight=64;
    pp.BackBufferFormat=D3DFMT_X8R8G8B8;pp.EnableAutoDepthStencil=TRUE;
    pp.AutoDepthStencilFormat=D3DFMT_D24S8;
    IDirect3DDevice9* d=nullptr;
    if(ex){IDirect3DDevice9Ex* e=nullptr;assert(SUCCEEDED(apiEx->CreateDeviceEx(0,D3DDEVTYPE_HAL,w,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING,&pp,nullptr,&e)));d=e;}
    else {const auto hr=api->CreateDevice(0,D3DDEVTYPE_HAL,w,D3DCREATE_SOFTWARE_VERTEXPROCESSING,&pp,&d);
        if(FAILED(hr))fprintf(stderr,"D3D9 HAL CreateDevice failed HRESULT=%08lX adapters=%u\n",hr,api->GetAdapterCount());
        assert(SUCCEEDED(hr));}
    trackedprime::Cache cache;
    IDirect3DVertexBuffer9* vb=nullptr;IDirect3DIndexBuffer9* ib=nullptr;
    auto resources=[&]{
        assert(SUCCEEDED(d->CreateVertexBuffer(sizeof(V)*4,D3DUSAGE_WRITEONLY,D3DFVF_XYZ|D3DFVF_DIFFUSE,
            D3DPOOL_DEFAULT,&vb,nullptr)));
        assert(SUCCEEDED(d->CreateIndexBuffer(6*sizeof(WORD),D3DUSAGE_WRITEONLY,D3DFMT_INDEX16,
            D3DPOOL_DEFAULT,&ib,nullptr)));
        const V vertices[]={{-.75f,-.5f,.2f,0xffffffff},{-.25f,-.5f,.2f,0xffffffff},
            {-.75f,.5f,.2f,0xffffffff},{-.25f,.5f,.2f,0xffffffff}};
        void* p=nullptr;assert(SUCCEEDED(vb->Lock(0,0,&p,0)));memcpy(p,vertices,sizeof(vertices));vb->Unlock();
        const WORD idx[]={0,2,1,1,2,3};assert(SUCCEEDED(ib->Lock(0,0,&p,0)));memcpy(p,idx,sizeof(idx));ib->Unlock();
        d->SetStreamSource(0,vb,0,sizeof(V));d->SetIndices(ib);d->SetFVF(D3DFVF_XYZ|D3DFVF_DIFFUSE);
        d->SetRenderState(D3DRS_LIGHTING,FALSE);d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
        d->SetRenderState(D3DRS_ZENABLE,D3DZB_TRUE);d->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);
        d->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);d->SetRenderState(D3DRS_STENCILENABLE,FALSE);
        d->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);
        d->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);
        d->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);
        D3DMATRIX m{};m._11=m._22=m._33=m._44=1;
        d->SetTransform(D3DTS_WORLD,&m);d->SetTransform(D3DTS_VIEW,&m);d->SetTransform(D3DTS_PROJECTION,&m);
    };
    resources();
    trackedprime::Owner owner{1,2,10,true};trackedprime::Draw color;
    color.triangles=true;color.foreground=true;color.key.verts=4;color.key.prims=2;
    auto prime=color;prime.foreground=false;
    auto viewport=[&](float max){D3DVIEWPORT9 vp{0,0,64,64,0,max};assert(SUCCEEDED(d->SetViewport(&vp)));};
    auto learn=[&]{viewport(1);d->SetRenderState(D3DRS_COLORWRITEENABLE,15);
        assert(SUCCEEDED(d->BeginScene()));const HRESULT h=d->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,4,0,2);
        assert(SUCCEEDED(d->EndScene()));return cache.Learn(d,owner,color,SUCCEEDED(h));};
    auto primeState=[&]{D3DVIEWPORT9 vp{0,0,64,64,0,0};const auto h=d->SetViewport(&vp);assert(SUCCEEDED(h));cache.ObserveViewport(d,&vp,h);d->SetRenderState(D3DRS_COLORWRITEENABLE,0);};
    primeState();assert(!cache.Skip(d,owner,prime));
    assert(learn());primeState();
    {D3DVIEWPORT9 v{};d->GetViewport(&v);
        printf("Native requested viewport 0/0 -> driver actual %g/%g\n",v.MinZ,v.MaxZ);}
    assert(cache.Skip(d,owner,prime));
    trackedprime::Inspection evidence;
    assert(cache.Inspect(d,owner,prime,evidence)&&evidence.exact&&
        !strcmp(evidence.gate,"eligible")&&evidence.key.ib==ib&&evidence.key.vb==vb);
    viewport(1);cache.ObserveViewport(d,nullptr,E_FAIL);
    assert(cache.Inspect(d,owner,prime,evidence)&&evidence.exact&&
        !strcmp(evidence.gate,"no-native-zero-viewport"));
    primeState();d->SetRenderState(D3DRS_COLORWRITEENABLE,15);
    assert(cache.Inspect(d,owner,prime,evidence)&&!strcmp(evidence.gate,"color-write-on"));
    d->SetRenderState(D3DRS_COLORWRITEENABLE,0);
    // A failed native SetViewport, or a nonzero *requested* depth range,
    // cannot seed the prime witness even if this driver canonicalizes it.
    D3DVIEWPORT9 request{0,0,64,64,0,0};
    cache.ObserveViewport(d,&request,E_FAIL);assert(!cache.Skip(d,owner,prime));
    request.MaxZ=.001f;cache.ObserveViewport(d,&request,S_OK);assert(!cache.Skip(d,owner,prime));
    primeState();assert(cache.Skip(d,owner,prime));
    // Fail closed on all state/identity/lifetime/context differences.
    auto other=owner;other.active=false;assert(!cache.Skip(d,other,prime));
    auto bad=prime;bad.key.layoutShadow=true;assert(!cache.Skip(d,owner,bad));
    bad=prime;bad.key.rtIsScene=false;assert(!cache.Skip(d,owner,bad));
    bad=prime;bad.foreground=true;assert(!cache.Skip(d,owner,bad));
    bad=prime;bad.triangles=false;assert(!cache.Skip(d,owner,bad));
    bad=prime;bad.key.startIndex=3;assert(!cache.Skip(d,owner,bad));
    bad=prime;bad.key.baseVertex=1;assert(!cache.Skip(d,owner,bad));
    bad=prime;bad.key.minIndex=1;assert(!cache.Skip(d,owner,bad));
    bad=prime;bad.key.verts=3;assert(!cache.Skip(d,owner,bad));
    bad=prime;bad.key.prims=1;assert(!cache.Skip(d,owner,bad));
    d->SetRenderState(D3DRS_COLORWRITEENABLE,15);assert(!cache.Skip(d,owner,prime));
    d->SetRenderState(D3DRS_COLORWRITEENABLE,0);d->SetRenderState(D3DRS_STENCILENABLE,TRUE);
    assert(!cache.Skip(d,owner,prime));d->SetRenderState(D3DRS_STENCILENABLE,FALSE);
    d->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);assert(!cache.Skip(d,owner,prime));d->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);
    d->SetRenderState(D3DRS_ZENABLE,D3DZB_FALSE);assert(!cache.Skip(d,owner,prime));d->SetRenderState(D3DRS_ZENABLE,D3DZB_TRUE);
    viewport(1);assert(!cache.Skip(d,owner,prime));primeState();
    d->SetIndices(nullptr);assert(!cache.Skip(d,owner,prime));d->SetIndices(ib);
    d->SetStreamSource(0,nullptr,0,0);assert(!cache.Skip(d,owner,prime));d->SetStreamSource(0,vb,0,sizeof(V));
    other=owner;other.frame+=3;assert(!cache.Skip(d,other,prime));
    other=owner;other.weapon++;assert(!cache.Skip(d,other,prime));assert(learn());
    other=owner;other.pawn++;assert(!cache.Skip(d,other,prime));
    viewport(1);d->SetRenderState(D3DRS_COLORWRITEENABLE,15);
    assert(!cache.Learn(d,owner,color,false));primeState();assert(!cache.Skip(d,owner,prime));assert(learn());

    DWORD oldPixel[2][2]{},gunPixel[2][2]{};
    for(unsigned fixed=0;fixed<2;++fixed){
        assert(SUCCEEDED(d->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff000000,1,0)));
        assert(SUCCEEDED(d->BeginScene()));
        for(unsigned eye=0;eye<2;++eye){
            D3DMATRIX m{};m._11=m._22=m._33=m._44=1;d->SetTransform(D3DTS_WORLD,&m);
            D3DVIEWPORT9 vp{eye*64,0,64,64,0,0};const auto vh=d->SetViewport(&vp);
            assert(SUCCEEDED(vh));cache.ObserveViewport(d,&vp,vh);
            d->SetRenderState(D3DRS_COLORWRITEENABLE,0);
            auto enabled=owner;enabled.active=fixed!=0;
            if(!cache.Skip(d,enabled,prime))assert(SUCCEEDED(d->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,4,0,2)));
            // World fills the view at z=.8, except where stale prime wrote z=0.
            vp.MaxZ=1;d->SetViewport(&vp);d->SetRenderState(D3DRS_COLORWRITEENABLE,15);
            const V world[]={{-1,-1,.8f,0xff204080},{-1,1,.8f,0xff204080},
                {1,-1,.8f,0xff204080},{1,1,.8f,0xff204080}};
            assert(SUCCEEDED(d->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,world,sizeof(V))));
            // DrawPrimitiveUP unbinds stream 0. Native late pass binds it again.
            d->SetStreamSource(0,vb,0,sizeof(V));d->SetIndices(ib);
            assert(SUCCEEDED(d->Clear(0,nullptr,D3DCLEAR_ZBUFFER,0,1,0)));
            m._41=1;d->SetTransform(D3DTS_WORLD,&m);
            assert(SUCCEEDED(d->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,4,0,2)));
        }
        assert(SUCCEEDED(d->EndScene()));
        IDirect3DSurface9 *rt=nullptr,*read=nullptr;d->GetRenderTarget(0,&rt);
        assert(SUCCEEDED(d->CreateOffscreenPlainSurface(128,64,D3DFMT_X8R8G8B8,D3DPOOL_SYSTEMMEM,&read,nullptr)));
        assert(SUCCEEDED(d->GetRenderTargetData(rt,read)));D3DLOCKED_RECT lock{};
        assert(SUCCEEDED(read->LockRect(&lock,nullptr,D3DLOCK_READONLY)));
        for(unsigned eye=0;eye<2;++eye){auto* row=(DWORD*)((char*)lock.pBits+32*lock.Pitch);
            oldPixel[fixed][eye]=row[eye*64+16]&0xffffff;gunPixel[fixed][eye]=row[eye*64+48]&0xffffff;}
        read->UnlockRect();read->Release();rt->Release();
    }
    for(unsigned eye=0;eye<2;++eye){assert(oldPixel[0][eye]==0);assert(oldPixel[1][eye]==0x204080);
        assert(gunPixel[0][eye]==0xffffff&&gunPixel[1][eye]==0xffffff);}
    // Queries/dispatch preserve the exact caller bindings and all tested state.
    primeState();IDirect3DIndexBuffer9* bound=nullptr;d->GetIndices(&bound);assert(bound==ib);bound->Release();
    assert(cache.Skip(d,owner,prime));D3DVIEWPORT9 after{};d->GetViewport(&after);assert(after.MinZ==0);
    DWORD state=~0u;d->GetRenderState(D3DRS_COLORWRITEENABLE,&state);assert(state==0);
    cache.Reset();assert(!cache.Skip(d,owner,prime));
    d->SetIndices(nullptr);d->SetStreamSource(0,nullptr,0,0);
    assert(vb->Release()==0&&ib->Release()==0);vb=nullptr;ib=nullptr;
    assert(SUCCEEDED(d->Reset(&pp)));resources();primeState();assert(!cache.Skip(d,owner,prime));
    assert(learn());primeState();assert(cache.Skip(d,owner,prime));cache.Reset();
    d->SetIndices(nullptr);d->SetStreamSource(0,nullptr,0,0);assert(vb->Release()==0&&ib->Release()==0);
    d->Release();api->Release();DestroyWindow(w);
    printf("PASS D3D9%s: exact sibling + OFF/identity/state/lifetime/Reset; both-eye pixels black->204080, gun FFFFFF unchanged\n",ex?"Ex":"");
}
int main(){Run(false);Run(true);}
