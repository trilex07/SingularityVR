#pragma once
// Default-OFF, read-only FINAL driver-call census. One requested gameplay frame.
// No bytecode query, allocation or disk logging in the draw path. COM references
// retained only for bounded unique groups and released by Present/reset.
namespace shadowtrace {
constexpr unsigned kGroups=1024, kDraws=24000, kShaders=192;
constexpr D3DRENDERSTATETYPE kPipelineStates[]={D3DRS_COLORWRITEENABLE,D3DRS_ZENABLE,
    D3DRS_ZWRITEENABLE,D3DRS_ZFUNC,D3DRS_STENCILENABLE,D3DRS_STENCILFUNC,D3DRS_STENCILREF,
    D3DRS_ALPHABLENDENABLE,D3DRS_SRCBLEND,D3DRS_DESTBLEND,D3DRS_SEPARATEALPHABLENDENABLE,
    D3DRS_SRCBLENDALPHA,D3DRS_DESTBLENDALPHA};
struct Key {
    IDirect3DVertexShader9* vs=nullptr;
    IDirect3DPixelShader9* ps=nullptr;
    IDirect3DSurface9* rt=nullptr;
    IDirect3DBaseTexture9* tex[8]{};
    RECT sc{};
    unsigned phase=0,moved=0;
    DWORD pipeline[13]{};
    bool pipelineValid=false;
};
struct Group {
    Key key; D3DVIEWPORT9 vp{}; IDirect3DSurface9* depth=nullptr;
    unsigned first=0,last=0,count=0,failed=0,type=0,verts=0,minPrims=~0u,maxPrims=0;
    float vsConstants[32][4]{},psConstants[32][4]{};
    HRESULT vsRead=E_FAIL,psRead=E_FAIL;
    float tailVS[4][4]{},tailPS[15][4]{};
    HRESULT tailVSRead=E_FAIL,tailPSRead=E_FAIL;
    bool tail=false;
    IDirect3DVertexBuffer9* vb=nullptr;IDirect3DIndexBuffer9* ib=nullptr;
    UINT offset=0,stride=0;
};
static Key bound;
static Group groups[kGroups];
static unsigned count=0,draws=0,overflow=0,sequence=0,tailReads=0;
static bool active=false;
static bool pending=false;
inline void Release() {
    for(unsigned i=0;i<count;++i) {
        auto& g=groups[i];
        if(g.key.vs)g.key.vs->Release(); if(g.key.ps)g.key.ps->Release();
        if(g.key.rt)g.key.rt->Release(); if(g.depth)g.depth->Release();
        if(g.vb)g.vb->Release();if(g.ib)g.ib->Release();
        for(auto t:g.key.tex)if(t)t->Release();
    }
    count=draws=overflow=tailReads=0;active=false;
}
inline int Before(IDirect3DDevice9* dev,unsigned type,unsigned verts,unsigned prims,unsigned phase,unsigned moved=0) {
    if(!active || draws>=kDraws) { if(active)++overflow; return -1; }
    ++draws;
    Key key=bound; // Shader/resource identities tracked at successful setters.
    key.phase=phase;
    key.moved=moved;
    key.pipelineValid=true;
    for(unsigned r=0;r<13;++r)if(FAILED(dev->GetRenderState(kPipelineStates[r],&key.pipeline[r]))) {
        key.pipeline[r]=~0u;key.pipelineValid=false;
    }
    if(FAILED(dev->GetScissorRect(&key.sc))) { ++overflow;return -1; }
    unsigned i=0;
    for(;i<count;++i)if(memcmp(&groups[i].key,&key,sizeof(Key))==0)break;
    if(i==count) {
        if(count==kGroups){++overflow;return -1;}
        auto& g=groups[count++];g={};g.key=key;g.first=draws;g.type=type;g.verts=verts;
        if(key.vs)key.vs->AddRef(); if(key.ps)key.ps->AddRef(); if(key.rt)key.rt->AddRef();
        for(auto t:key.tex)if(t)t->AddRef();
        dev->GetViewport(&g.vp);dev->GetDepthStencilSurface(&g.depth);
        dev->GetStreamSource(0,&g.vb,&g.offset,&g.stride);dev->GetIndices(&g.ib);
        g.vsRead=dev->GetVertexShaderConstantF(0,&g.vsConstants[0][0],32);
        g.psRead=dev->GetPixelShaderConstantF(0,&g.psConstants[0][0],32);
    }
    auto& g=groups[i];++g.count;g.last=draws;
    // First and last volume draws can have different matrix semantics despite
    // identical COM state. Read only these small repeated volumes, <=128/frame.
    if(g.count>1 && verts==8 && prims==12 && tailReads<128) {
        ++tailReads;g.tail=true;
        g.tailVSRead=dev->GetVertexShaderConstantF(0,&g.tailVS[0][0],4);
        g.tailPSRead=dev->GetPixelShaderConstantF(0,&g.tailPS[0][0],15);
    }
    g.minPrims=std::min(g.minPrims,prims);g.maxPrims=std::max(g.maxPrims,prims);
    return (int)i;
}
inline void After(int i,HRESULT hr){if(i>=0 && FAILED(hr))++groups[i].failed;}
inline void WriteShader(FILE* f,IUnknown* shader,bool pixel) {
    UINT bytes=0; HRESULT hr=pixel?static_cast<IDirect3DPixelShader9*>(shader)->GetFunction(nullptr,&bytes)
        :static_cast<IDirect3DVertexShader9*>(shader)->GetFunction(nullptr,&bytes);
    static DWORD code[8192]; // Present-only render-thread storage, not draw-stack pressure.
    if(FAILED(hr)||bytes<8||bytes%4||bytes>sizeof(code)) {
        fprintf(f,"shader-unavailable %p stage=%s hr=%08lX bytes=%u\n",shader,pixel?"PS":"VS",hr,bytes);return;
    }
    const UINT capacity=bytes;
    hr=pixel?static_cast<IDirect3DPixelShader9*>(shader)->GetFunction(code,&bytes)
        :static_cast<IDirect3DVertexShader9*>(shader)->GetFunction(code,&bytes);
    if(FAILED(hr)||bytes>capacity||bytes%4)return;
    fprintf(f,"shader %p stage=%s words=%u\n",shader,pixel?"PS":"VS",bytes/4);
    for(unsigned i=0;i<bytes/4;++i)fprintf(f,"%08lX%c",code[i],(i%16==15)?'\n':' ');
    fputc('\n',f);
}
inline void Dump(const char* path) {
    FILE* f=nullptr;if(fopen_s(&f,path,"wb")||!f)return;
    fprintf(f,"shadow-pass-trace-v1 draws=%u groups=%u overflow=%u\n",draws,count,overflow);
    IUnknown* shaders[kShaders]{};unsigned sn=0;
    for(unsigned i=0;i<count;++i) {
        const auto& g=groups[i]; D3DSURFACE_DESC d{};if(g.key.rt)g.key.rt->GetDesc(&d);
        IDirect3DTexture9* owner=nullptr;
        if(g.key.rt)g.key.rt->GetContainer(__uuidof(IDirect3DTexture9),(void**)&owner);
        fprintf(f,"group=%u first=%u last=%u draws=%u failed=%u VS=%p PS=%p RT=%p owner=%p depth=%p size=%ux%u fmt=%u type=%u verts=%u prims=%u..%u vp=%lu,%lu,%lu,%lu sc=%ld,%ld,%ld,%ld vsRead=%08lX psRead=%08lX\n",
            i,g.first,g.last,g.count,g.failed,g.key.vs,g.key.ps,g.key.rt,owner,g.depth,d.Width,d.Height,(unsigned)d.Format,
            g.type,g.verts,g.minPrims,g.maxPrims,g.vp.X,g.vp.Y,g.vp.Width,g.vp.Height,
            g.key.sc.left,g.key.sc.top,g.key.sc.right,g.key.sc.bottom,g.vsRead,g.psRead);
        if(owner)owner->Release();
        fprintf(f,"geometry group=%u phase=%u moved=%u VB=%p offset=%u stride=%u IB=%p\n",i,g.key.phase,g.key.moved,g.vb,g.offset,g.stride,g.ib);
        fprintf(f,"pipeline group=%u valid=%u colorWrite=%lu zEnable=%lu zWrite=%lu zFunc=%lu stencil=%lu stencilFunc=%lu stencilRef=%lu blend=%lu srcBlend=%lu dstBlend=%lu separateAlpha=%lu srcAlpha=%lu dstAlpha=%lu\n",
            i,g.key.pipelineValid?1:0,g.key.pipeline[0],g.key.pipeline[1],g.key.pipeline[2],g.key.pipeline[3],
            g.key.pipeline[4],g.key.pipeline[5],g.key.pipeline[6],g.key.pipeline[7],g.key.pipeline[8],
            g.key.pipeline[9],g.key.pipeline[10],g.key.pipeline[11],g.key.pipeline[12]);
        if(g.tail) {
            fprintf(f,"tail group=%u draw=%u vsRead=%08lX psRead=%08lX\n",i,g.last,g.tailVSRead,g.tailPSRead);
            for(unsigned r=0;r<15;++r)fprintf(f,"tailconstant group=%u reg=%u VS=%.9g,%.9g,%.9g,%.9g PS=%.9g,%.9g,%.9g,%.9g\n",
                i,r,r<4?g.tailVS[r][0]:0,r<4?g.tailVS[r][1]:0,r<4?g.tailVS[r][2]:0,r<4?g.tailVS[r][3]:0,
                g.tailPS[r][0],g.tailPS[r][1],g.tailPS[r][2],g.tailPS[r][3]);
        }
        for(unsigned t=0;t<8;++t) {
            auto texture=g.key.tex[t];D3DSURFACE_DESC desc{};
            if(texture && texture->GetType()==D3DRTYPE_TEXTURE)
                static_cast<IDirect3DTexture9*>(texture)->GetLevelDesc(0,&desc);
            fprintf(f,"texture group=%u stage=%u id=%p size=%ux%u fmt=%u\n",i,t,texture,desc.Width,desc.Height,(unsigned)desc.Format);
        }
        for(unsigned r=0;r<32;++r)fprintf(f,"constant group=%u reg=%u VS=%.9g,%.9g,%.9g,%.9g PS=%.9g,%.9g,%.9g,%.9g\n",
            i,r,g.vsConstants[r][0],g.vsConstants[r][1],g.vsConstants[r][2],g.vsConstants[r][3],
            g.psConstants[r][0],g.psConstants[r][1],g.psConstants[r][2],g.psConstants[r][3]);
        IUnknown* candidates[]={g.key.vs,g.key.ps};
        for(unsigned stage=0;stage<2;++stage) {
            auto s=candidates[stage];if(!s)continue;
            unsigned n=0;for(;n<sn;++n)if(shaders[n]==s)break;
            if(n==sn && sn<kShaders){shaders[sn++]=s;WriteShader(f,s,stage==1);}
            else if(n==sn)fprintf(f,"shader-budget-overflow %p\n",s);
        }
    }
    fclose(f);
}
inline void Arm(IDirect3DDevice9* dev) {
    Release();bound={};
    dev->GetVertexShader(&bound.vs);if(bound.vs)bound.vs->Release();
    dev->GetPixelShader(&bound.ps);if(bound.ps)bound.ps->Release();
    dev->GetRenderTarget(0,&bound.rt);if(bound.rt)bound.rt->Release();
    for(unsigned t=0;t<8;++t){dev->GetTexture(t,&bound.tex[t]);if(bound.tex[t])bound.tex[t]->Release();}
    active=true;++sequence;
}
inline bool Boundary(IDirect3DDevice9*,bool gameplay,const char* logPath) {
    const bool captured=active;
    char dir[MAX_PATH]{};strncpy_s(dir,logPath,_TRUNCATE);
    char* end=strrchr(dir,'\\');if(!end)return false;end[1]=0;
    if(active) {
        active=false;
        char file[MAX_PATH]{};
        _snprintf_s(file,sizeof(file),_TRUNCATE,"%sshadow-pass-%lu-%u.txt",dir,GetCurrentProcessId(),sequence);
        Dump(file);Log("shadow-pass trace finished draws=%u groups=%u overflow=%u file=%s",draws,count,overflow,file);
        Release();
    }
    static DWORD last=0;const DWORD now=GetTickCount();
    if(!gameplay || sequence>=4 || now-last<1000)return captured;last=now;
    char request[MAX_PATH]{};_snprintf_s(request,sizeof(request),_TRUNCATE,"%sshadow-pass.request",dir);
    if(GetFileAttributesA(request)!=INVALID_FILE_ATTRIBUTES && DeleteFileA(request)) {
        pending=true;Log("shadow-pass trace armed: read-only one gameplay frame, budget=%u groups/%u draws",kGroups,kDraws);
    }
    return captured;
}
}
