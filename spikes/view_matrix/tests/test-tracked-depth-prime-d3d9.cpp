#define NOMINMAX
#include "../tracked_depth_prime.h"
#include "../tracked_depth_source.h"
#include "../dual_hand_action_pose.h"
#include "../arm_natural.h"
#include <cassert>
#include <cstdio>
#include <cmath>

// Real D3D9/Ex pixels through the production cache/dispatch predicate. Synthetic
// scene reproduces early depth-only old pose -> world -> late new pose. It does
// not establish the runtime identity of the user's black silhouette.
struct V {float x,y,z;DWORD color;};
static armbind::Mat Identity(){armbind::Mat m{};for(unsigned i=0;i<4;++i)m.m[i][i]=1;return m;}
static armbind::Mat Inverse(const armbind::Mat& m){armbind::Mat r;assert(skeletal::Inverse(r.m,m.m));return r;}
static float MatrixError(const armbind::Mat& a,const armbind::Mat& b){
    float e=0;for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j)e=fmaxf(e,fabsf(a.m[i][j]-b.m[i][j]));return e;
}
static void SourceOrderingPixels(IDirect3DDevice9* d,bool ex){
    // The actual cache must omit the early exact material-0 range, while its
    // original shader constants still reach the real source reader/Frame.
    // Geometry is synthetic (a repeated quad), not extracted proprietary data.
    trackedprime::Cache cache;
    trackedprime::Owner owner{11,22,100,true};
    trackedprime::Draw gunColor;gunColor.triangles=true;gunColor.foreground=true;
    gunColor.key.verts=4;gunColor.key.prims=2;
    auto gloveColor=gunColor;gloveColor.key.verts=7444;gloveColor.key.prims=4402;
    auto gunPrime=gunColor;gunPrime.foreground=false;
    auto glovePrime=gloveColor;glovePrime.foreground=false;
    IDirect3DVertexBuffer9 *gunVb=nullptr,*gloveVb=nullptr;
    IDirect3DIndexBuffer9 *gunIb=nullptr,*gloveIb=nullptr;
    const V quad[]={{-.75f,-.5f,.2f,0xffffffff},{-.25f,-.5f,.2f,0xffffffff},
        {-.75f,.5f,.2f,0xffffffff},{-.25f,.5f,.2f,0xffffffff}};
    auto makeBuffers=[&](unsigned vertices,unsigned triangles,IDirect3DVertexBuffer9** vb,IDirect3DIndexBuffer9** ib){
        assert(SUCCEEDED(d->CreateVertexBuffer(vertices*sizeof(V),D3DUSAGE_WRITEONLY,D3DFVF_XYZ|D3DFVF_DIFFUSE,D3DPOOL_DEFAULT,vb,nullptr)));
        assert(SUCCEEDED(d->CreateIndexBuffer(triangles*3*sizeof(WORD),D3DUSAGE_WRITEONLY,D3DFMT_INDEX16,D3DPOOL_DEFAULT,ib,nullptr)));
        void* p=nullptr;assert(SUCCEEDED((*vb)->Lock(0,0,&p,0)));memset(p,0,vertices*sizeof(V));memcpy(p,quad,sizeof(quad));(*vb)->Unlock();
        assert(SUCCEEDED((*ib)->Lock(0,0,&p,0)));const WORD indices[]={0,2,1,1,2,3};
        for(unsigned i=0;i<triangles*3;++i)((WORD*)p)[i]=indices[i%6];(*ib)->Unlock();
    };
    makeBuffers(4,2,&gunVb,&gunIb);makeBuffers(7444,4402,&gloveVb,&gloveIb);
    auto bind=[&](bool glove){assert(SUCCEEDED(d->SetStreamSource(0,glove?gloveVb:gunVb,0,sizeof(V))));assert(SUCCEEDED(d->SetIndices(glove?gloveIb:gunIb)));};
    auto draw=[&](bool glove){return d->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,glove?7444:4,0,glove?4402:2);};
    auto world=[&](const armbind::Mat& m){D3DMATRIX v;memcpy(&v,m.m,sizeof(v));assert(SUCCEEDED(d->SetTransform(D3DTS_WORLD,&v)));};
    const auto identity=Identity();world(identity);
    D3DVIEWPORT9 full{0,0,128,64,0,1};assert(SUCCEEDED(d->SetViewport(&full)));
    assert(SUCCEEDED(d->SetRenderState(D3DRS_COLORWRITEENABLE,15)));
    assert(SUCCEEDED(d->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL)));
    assert(SUCCEEDED(d->BeginScene()));
    bind(false);assert(SUCCEEDED(draw(false)));assert(cache.Learn(d,owner,gunColor,true));
    bind(true);assert(SUCCEEDED(draw(true)));assert(cache.Learn(d,owner,gloveColor,true));
    assert(SUCCEEDED(d->EndScene()));
    ++owner.frame;

    const auto& original=armnatural::TmdBind();assert(original.ready);
    auto wanted=identity;wanted.m[3][0]=.25f;wanted.m[3][2]=.2f;
    const auto nativeLeft=dualaction::Product(dualhandreference::Left,Inverse(wanted));
    const auto nativeRight=dualaction::Product(dualhandreference::Right8297,Inverse(wanted));
    trackedsource::Source source;
    memcpy(source.local,identity.m,sizeof(source.local));
    armbind::ToGpu(dualaction::Product(Inverse(original.bind[9]),nativeLeft),source.left);
    armbind::ToGpu(dualaction::Product(Inverse(original.bind[51]),nativeRight),source.right);
    assert(SUCCEEDED(d->SetVertexShaderConstantF(230,&source.local[0][0],4)));
    assert(SUCCEEDED(d->SetVertexShaderConstantF(8,&source.left[0][0],3)));
    assert(SUCCEEDED(d->SetVertexShaderConstantF(62,&source.right[0][0],3)));
    trackedsource::Input input;input.enabled=input.omitted=input.eligible=true;
    input.verts=7444;input.first=0;input.triangles=4402;input.bones=5;input.boneRegs=225;input.local=230;
    assert(trackedsource::Gate(input)==nullptr);
    auto subset=input;subset.verts=2626;assert(trackedsource::Gate(subset)==nullptr);
    const dualaction::Key key{owner.frame,(uint32_t)owner.pawn,(uint32_t)owner.weapon,8297};
    auto capture=[&](dualaction::Frame& f,const trackedsource::Input& in){
        trackedsource::Source s;const char* refusal=trackedsource::Read(d,in,s);
        if(refusal)return false;
        armbind::Mat local;memcpy(local.m,s.local,sizeof(local.m));
        return f.Capture(key,local,original.bind[9],s.left,original.bind[51],s.right);
    };
    // Refused paths never fabricate a source or mutate output, even if the
    // device is unavailable. They must not silently read guessed registers.
    for(unsigned gate=0;gate<9;++gate){auto bad=input;
        switch(gate){case 0:bad.enabled=false;break;case 1:bad.omitted=false;break;case 2:bad.eligible=false;break;
        case 3:bad.shadow=true;break;case 4:bad.verts=8297;break;case 5:bad.first=3;break;
        case 6:bad.triangles=4401;break;case 7:bad.bones=4;break;case 8:bad.local=229;break;}
        trackedsource::Source output;memset(&output,0x5a,sizeof(output));const auto before=output;
        assert(trackedsource::Gate(bad)!=nullptr&&trackedsource::Read(nullptr,bad,output)!=nullptr);
        assert(memcmp(&before,&output,sizeof(output))==0);
        dualaction::Frame f;assert(!capture(f,bad)&&!f.valid);
    }
    {auto bad=input;bad.boneRegs=224;assert(trackedsource::Gate(bad)!=nullptr);}
    dualaction::Frame unsupported;
    assert(!unsupported.Capture({owner.frame,11,22,12444},identity,original.bind[9],source.left,original.bind[51],source.right));
    assert(!unsupported.valid); // no Seeker profile is fabricated

    DWORD overlap[2][2][2]{},extension[2][2][2]{};
    for(unsigned equal=0;equal<2;++equal)for(unsigned fixed=0;fixed<2;++fixed){
        assert(SUCCEEDED(d->SetViewport(&full)));world(identity);
        assert(SUCCEEDED(d->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff204080,1,0)));
        assert(SUCCEEDED(d->BeginScene()));
        for(unsigned eye=0;eye<2;++eye){
            dualaction::Frame frame;
            // Stale previous-frame witness must not be accepted as current.
            assert(capture(frame,input));--frame.key.frame;
            D3DVIEWPORT9 vp{eye*64,0,64,64,0,.0001f};auto h=d->SetViewport(&vp);assert(SUCCEEDED(h));cache.ObserveViewport(d,&vp,h);
            assert(SUCCEEDED(d->SetRenderState(D3DRS_COLORWRITEENABLE,0)));
            bind(false);assert(cache.Skip(d,owner,gunPrime));
            bind(true);auto early=input;early.omitted=cache.Skip(d,owner,glovePrime);assert(early.omitted);
            if(fixed)assert(capture(frame,early));
            vp.MaxZ=1;h=d->SetViewport(&vp);assert(SUCCEEDED(h));cache.ObserveViewport(d,&vp,h);
            // A genuine late foreground clear/prepass remains present. The fix
            // repairs its transform instead of suppressing required depth.
            assert(SUCCEEDED(d->Clear(0,nullptr,D3DCLEAR_ZBUFFER,0,1,0)));
            bind(false);assert(!cache.Skip(d,owner,gunPrime));
            auto depthC=identity;assert(frame.Apply(key,true,depthC.m)==(fixed!=0));
            world(depthC);assert(SUCCEEDED(d->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL)));
            assert(SUCCEEDED(draw(false)));
            // Later original material0 arrives before final gun color. It must
            // agree with the source retained from the omitted prime.
            bind(true);assert(capture(frame,input));
            auto colorC=identity;assert(frame.Apply(key,true,colorC.m));
            assert(MatrixError(colorC,wanted)<.0001f);
            if(fixed)assert(MatrixError(depthC,colorC)==0);
            else assert(MatrixError(depthC,colorC)>.1f);
            bind(false);world(colorC);assert(SUCCEEDED(d->SetRenderState(D3DRS_COLORWRITEENABLE,15)));
            assert(SUCCEEDED(d->SetRenderState(D3DRS_ZFUNC,equal?D3DCMP_EQUAL:D3DCMP_LESSEQUAL)));
            assert(SUCCEEDED(draw(false)));
        }
        assert(SUCCEEDED(d->EndScene()));
        IDirect3DSurface9 *rt=nullptr,*read=nullptr;assert(SUCCEEDED(d->GetRenderTarget(0,&rt)));
        assert(SUCCEEDED(d->CreateOffscreenPlainSurface(128,64,D3DFMT_X8R8G8B8,D3DPOOL_SYSTEMMEM,&read,nullptr)));
        assert(SUCCEEDED(d->GetRenderTargetData(rt,read)));D3DLOCKED_RECT lock{};assert(SUCCEEDED(read->LockRect(&lock,nullptr,D3DLOCK_READONLY)));
        const auto* row=(DWORD*)((char*)lock.pBits+32*lock.Pitch);
        for(unsigned eye=0;eye<2;++eye){
            overlap[equal][fixed][eye]=row[eye*64+20]&0xffffff;
            extension[equal][fixed][eye]=row[eye*64+28]&0xffffff;
            assert((row[eye*64+60]&0xffffff)==0x204080);
        }
        read->UnlockRect();read->Release();rt->Release();
    }
    for(unsigned equal=0;equal<2;++equal)for(unsigned eye=0;eye<2;++eye){
        assert(overlap[equal][0][eye]==0x204080&&overlap[equal][1][eye]==0xffffff);
        assert(extension[equal][0][eye]==(equal?0x204080:0xffffff));
        assert(extension[equal][1][eye]==0xffffff);
        printf("source ordering %s %s eye=%u overlap=%06lX->%06lX extension=%06lX->%06lX\n",
            ex?"Ex":"D3D9",equal?"EQUAL":"LEQUAL",eye,overlap[equal][0][eye],overlap[equal][1][eye],extension[equal][0][eye],extension[equal][1][eye]);
    }
    // Read/capture must leave the original skin/local constants and bindings intact.
    trackedsource::Source after;assert(trackedsource::Read(d,input,after)==nullptr);
    assert(memcmp(&after,&source,sizeof(source))==0);
    IDirect3DIndexBuffer9* ib=nullptr;assert(SUCCEEDED(d->GetIndices(&ib))&&ib==gunIb);ib->Release();
    cache.Reset();d->SetIndices(nullptr);d->SetStreamSource(0,nullptr,0,0);
    assert(gunVb->Release()==0&&gunIb->Release()==0&&gloveVb->Release()==0&&gloveIb->Release()==0);
    assert(SUCCEEDED(d->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL)));
}
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
    auto primeState=[&](float max=.0001f){D3DVIEWPORT9 vp{0,0,64,64,0,max};const auto h=d->SetViewport(&vp);assert(SUCCEEDED(h));cache.ObserveViewport(d,&vp,h);d->SetRenderState(D3DRS_COLORWRITEENABLE,0);};
    primeState();assert(!cache.Skip(d,owner,prime));
    assert(learn());primeState();
    {D3DVIEWPORT9 v{};d->GetViewport(&v);
        printf("Native requested viewport 0/.0001 -> driver actual %g/%g\n",v.MinZ,v.MaxZ);}
    assert(cache.Skip(d,owner,prime));
    trackedprime::Inspection evidence;
    assert(cache.Inspect(d,owner,prime,evidence)&&evidence.exact&&
        !strcmp(evidence.gate,"eligible")&&evidence.key.ib==ib&&evidence.key.vb==vb);
    viewport(1);cache.ObserveViewport(d,nullptr,E_FAIL);
    assert(cache.Inspect(d,owner,prime,evidence)&&evidence.exact&&
        !strcmp(evidence.gate,"no-native-prime-viewport"));
    primeState();d->SetRenderState(D3DRS_COLORWRITEENABLE,15);
    assert(cache.Inspect(d,owner,prime,evidence)&&!strcmp(evidence.gate,"color-write-on"));
    d->SetRenderState(D3DRS_COLORWRITEENABLE,0);
    // Only the two evidenced native requests may seed the witness. The driver's
    // canonicalized .001 and even the adjacent float to .0001 are NOT requests.
    D3DVIEWPORT9 request{0,0,64,64,0,0};
    cache.ObserveViewport(d,&request,E_FAIL);assert(!cache.Skip(d,owner,prime));
    request.MaxZ=.001f;cache.ObserveViewport(d,&request,S_OK);assert(!cache.Skip(d,owner,prime));
    request.MaxZ=std::nextafter(.0001f,1.f);cache.ObserveViewport(d,&request,S_OK);assert(!cache.Skip(d,owner,prime));
    primeState(0.f);assert(cache.Skip(d,owner,prime));
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
            // Both measured requests, one per eye. The screenshot regression
            // used .0001, not the earlier synthetic 0/0 assumption.
            D3DVIEWPORT9 vp{eye*64,0,64,64,0,eye?.0001f:0.f};const auto vh=d->SetViewport(&vp);
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
    SourceOrderingPixels(d,ex);
    d->SetStreamSource(0,vb,0,sizeof(V));d->SetIndices(ib);
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
    printf("PASS D3D9%s: exact sibling + OFF/identity/state/lifetime/Reset; both-eye pixels black->204080; source-before-depth/color LEQUAL+EQUAL hole->FFFFFF, original constants preserved\n",ex?"Ex":"");
}
int main(){Run(false);Run(true);}
