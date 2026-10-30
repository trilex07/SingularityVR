#pragma once
#include <cmath>
#include <cstring>

// Exact-scene depth attachment split, not a depth copy. Native world draws
// keep their original D24S8; the original late Clear and foreground draws use
// one compatible private D24S8. Only binds of that exact native surface are
// redirected until the HUD completes. Shadow/decal targets remain native.
namespace wristdepth {
// Private metadata is tied to the COM resource lifetime, not a recycled address.
static const GUID kDepthDiscard={0x5e09cd43,0x2a71,0x47e8,{0x9c,0x63,0x42,0x6d,0xd1,0x50,0x92,0x73}};
inline void Created(IDirect3DSurface9* surface,BOOL discard){
    if(surface)surface->SetPrivateData(kDepthDiscard,&discard,sizeof discard,0);
}
inline bool Preservable(IDirect3DSurface9* surface){
    BOOL discard=TRUE;DWORD bytes=sizeof discard;
    return surface&&SUCCEEDED(surface->GetPrivateData(kDepthDiscard,&discard,&bytes))&&
        bytes==sizeof discard&&!discard;
}
using SetDepth=HRESULT (STDMETHODCALLTYPE*)(IDirect3DDevice9*,IDirect3DSurface9*);
struct Camera { float m[16]{};int conv=0;D3DVIEWPORT9 vp{};bool valid=false; };
struct Projected { float x=0,y=0,z=0,rhw=0,u=0,v=0; };
inline void QuarterTurn(float& x,float& y){const float old=x;x=y;y=1.f-old;}
// A screen-mask quad needs the same projective W up to one common scale.
// Refuse different world/foreground projection families instead of warping it.
inline bool CompatibleW(const Projected* world,const Projected* foreground){
    if(!world||!foreground||world[0].rhw<=0||foreground[0].rhw<=0)return false;
    const float ratio=world[0].rhw/foreground[0].rhw;
    for(int k=0;k<4;++k)if(!std::isfinite(world[k].rhw)||!std::isfinite(foreground[k].rhw)||
        foreground[k].rhw<=0||std::fabs(world[k].rhw/foreground[k].rhw-ratio)>std::fabs(ratio)*.0001f)return false;
    return true;
}
using DrawUP=HRESULT (STDMETHODCALLTYPE*)(IDirect3DDevice9*,D3DPRIMITIVETYPE,UINT,const void*,UINT);
inline HRESULT CompositeEye(IDirect3DDevice9* dev,DrawUP draw,int eye,UINT width,UINT height,
                            const Projected* world,const Projected* foreground){
    if(!CompatibleW(world,foreground))return D3DERR_INVALIDCALL;
    RECT clip{LONG(eye*width/2),0,LONG((eye+1)*width/2),LONG(height)};
    if(FAILED(dev->SetScissorRect(&clip)))return D3DERR_INVALIDCALL;
    // UVs already denote pixels in a screen-space mask. Perspective-correct
    // interpolation would apply W a second time and sample unrelated/empty
    // mask pixels on a tilted wrist. Z remains the native projected depth.
    struct V{float x,y,z,rhw,u,v;};V v[4];
    for(int k=0;k<4;++k){const auto& a=foreground[k];const auto& b=world[k];v[k]={a.x,a.y,a.z,1.f,b.u,b.v};}
    return draw(dev,D3DPT_TRIANGLESTRIP,2,v,sizeof(V));
}
// Bounded, asynchronous visibility evidence. Never flushes or waits for GPU.
// Stage 0 counts world-visible mask samples; stage 1 counts nontransparent
// composite samples surviving foreground Z. One measured HUD submission/run.
struct PixelWitness {
    IDirect3DQuery9* q[2][2]{};DWORD pixels[2][2]{};
    bool issued[2][2]{},complete[2][2]{},started=false,done=false;unsigned polls=0;
    void Release(){for(auto& phase:q)for(auto& query:phase){if(query)query->Release();query=nullptr;}}
    bool Start(IDirect3DDevice9* dev,bool enabled){
        if(!enabled||started||done)return false;started=true;
        for(auto& phase:q)for(auto& query:phase)
            if(FAILED(dev->CreateQuery(D3DQUERYTYPE_OCCLUSION,&query))){Release();done=true;return false;}
        return true;
    }
    void Begin(int phase,int eye){if(q[phase][eye])issued[phase][eye]=SUCCEEDED(q[phase][eye]->Issue(D3DISSUE_BEGIN));}
    void End(int phase,int eye){if(q[phase][eye]&&issued[phase][eye])
        issued[phase][eye]=SUCCEEDED(q[phase][eye]->Issue(D3DISSUE_END));}
    // Call once at a later Present; flags=0 explicitly excludes D3DGETDATA_FLUSH.
    bool Poll(){
        if(!started||done)return false;bool all=true;
        for(int p=0;p<2;++p)for(int e=0;e<2;++e){
            if(!issued[p][e]){done=true;Release();return false;}
            if(!complete[p][e]){const auto hr=q[p][e]->GetData(&pixels[p][e],sizeof(DWORD),0);
                if(FAILED(hr)){done=true;Release();return false;}complete[p][e]=hr==S_OK;}
            all=all&&complete[p][e];
        }
        if(all){done=true;Release();return true;}
        if(++polls>=120){done=true;Release();}return false;
    }
};
static PixelWitness pixelWitness;
struct State {
    IDirect3DSurface9 *world=nullptr,*foreground=nullptr,*maskSurface=nullptr;
    IDirect3DTexture9* mask=nullptr;
    D3DSURFACE_DESC desc{};
    Camera cameras[2][2]{}; // phase, eye; BEFORE controller ride
    unsigned attempts[2][2]{};
    bool active=false,failed=false,internal=false,enabled=false;
    unsigned frame=0;
    SetDepth native=nullptr;
    void Release(){
        if(world)world->Release();world=nullptr;
        if(foreground)foreground->Release();foreground=nullptr;
        if(maskSurface)maskSurface->Release();maskSurface=nullptr;
        if(mask)mask->Release();mask=nullptr;
        desc={};active=false;failed=false;std::memset(cameras,0,sizeof cameras);
    }
    void NewFrame(IDirect3DDevice9* dev){
        if(active&&native){IDirect3DSurface9* bound=nullptr;
            if(SUCCEEDED(dev->GetDepthStencilSurface(&bound))&&bound==foreground)
                native(dev,world);
            if(bound)bound->Release();}
        active=false;failed=false;std::memset(cameras,0,sizeof cameras);std::memset(attempts,0,sizeof attempts);++frame;
    }
    HRESULT Bind(IDirect3DDevice9* dev,IDirect3DSurface9* surface){
        if(!native)return D3DERR_INVALIDCALL;
        return native(dev,enabled&&active&&!internal&&surface==world?foreground:surface);
    }
    bool Compatible(const D3DSURFACE_DESC& d)const{
        return d.Format==D3DFMT_D24S8&&d.MultiSampleType==D3DMULTISAMPLE_NONE&&
               d.MultiSampleQuality==0&&d.Width>=8&&d.Height>=8&&(d.Width%2)==0;
    }
    void Eye(IDirect3DDevice9* dev,int phase,int eye,const float* m,int conv,const D3DVIEWPORT9& vp){
        if(!enabled||failed||phase<0||phase>1||eye<0||eye>1||!m||cameras[phase][eye].valid||
           attempts[phase][eye]>=8)return;
        ++attempts[phase][eye];
        IDirect3DSurface9* ds=nullptr;
        if(FAILED(dev->GetDepthStencilSurface(&ds))||!ds)return;
        D3DSURFACE_DESC d{};const bool compatible=SUCCEEDED(ds->GetDesc(&d))&&Compatible(d)&&
            vp.X==0&&vp.Y==0&&vp.Width==d.Width&&vp.Height==d.Height&&vp.MinZ==0&&vp.MaxZ==1;
        bool accept=false;
        if(!phase&&!active&&compatible&&Preservable(ds)){
            if(world!=ds){Release();world=ds;world->AddRef();desc=d;}
            accept=true;
        }else if(phase&&active&&ds==foreground&&compatible)accept=true;
        ds->Release();if(!accept)return;
        for(int i=0;i<16;++i)if(!std::isfinite(m[i]))return;
        auto& c=cameras[phase][eye];std::memcpy(c.m,m,sizeof c.m);c.conv=conv;c.vp=vp;c.valid=true;
    }
    // Called immediately BEFORE the native Clear. Requires both actual world
    // eye submissions and the exact same scene attachment/viewport. No draw
    // index threshold, resource-size-only identity or shadow classification.
    bool BeforeClear(IDirect3DDevice9* dev,bool foregroundBoundary,DWORD count,DWORD flags){
        if(!enabled||failed||!(flags&D3DCLEAR_ZBUFFER)||!foregroundBoundary)return false;
        IDirect3DSurface9* bound=nullptr;dev->GetDepthStencilSurface(&bound);
        const bool exact=bound&&bound==world; const bool isForeground=bound&&bound==foreground; if(bound)bound->Release();
        if(active){if(exact||isForeground)failed=true;return false;}
        // The measured boundary initializes both Z and stencil. Do not lose
        // native stencil contents by redirecting a Z-only clear.
        if(!exact||count||(flags&(D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL))!=(D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL)||
           !cameras[0][0].valid||!cameras[0][1].valid)return false;
        D3DVIEWPORT9 vp{};
        if(FAILED(dev->GetViewport(&vp))||vp.X||vp.Y||vp.Width!=desc.Width||vp.Height!=desc.Height||vp.MinZ!=0||vp.MaxZ!=1)return false;
        if(!foreground&&FAILED(dev->CreateDepthStencilSurface(desc.Width,desc.Height,desc.Format,
            D3DMULTISAMPLE_NONE,0,FALSE,&foreground,nullptr))){failed=true;return false;}
        if(!mask&&FAILED(dev->CreateTexture(desc.Width,desc.Height,1,D3DUSAGE_RENDERTARGET,
            D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&mask,nullptr))){failed=true;return false;}
        if(!maskSurface&&FAILED(mask->GetSurfaceLevel(0,&maskSurface))){failed=true;return false;}
        if(!native||FAILED(native(dev,foreground))){failed=true;return false;}
        active=true;return true;
    }
    bool Ready()const{
        return enabled&&active&&!failed&&world&&foreground&&maskSurface&&mask&&
            cameras[0][0].valid&&cameras[0][1].valid&&cameras[1][0].valid&&cameras[1][1].valid;
    }
    bool Project(int phase,int eye,const float p[3],Projected& out)const{
        if(phase<0||phase>1||eye<0||eye>1)return false;
        const auto& c=cameras[phase][eye];if(!c.valid)return false;
        float clip[4]{};
        for(int j=0;j<4;++j){for(int i=0;i<3;++i)clip[j]+=p[i]*c.m[c.conv==0?i*4+j:j*4+i];
            clip[j]+=c.m[c.conv==0?12+j:j*4+3];}
        for(float v:clip)if(!std::isfinite(v))return false;
        if(clip[3]<=.001f)return false;
        const float z=clip[2]/clip[3];if(z<0||z>1)return false;
        const float u=clip[0]/clip[3]*.5f+.5f,v=.5f-clip[1]/clip[3]*.5f;
        if(u<eye*.5f||u>(eye+1)*.5f||v<0||v>1)return false;
        out={u*c.vp.Width-.5f,v*c.vp.Height-.5f,z,1.f/clip[3],u,v};return true;
    }
};
static State state;
}
