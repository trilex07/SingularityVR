#pragma once
// Diagnostic only: fixed storage, no file I/O/readback/allocations in draw callbacks.
// Samples eight complete frames per native movie generation. Counts describe sampled
// frames, not the whole session. Resource addresses identify THIS process only.
volatile LONG g_menuDrawTrace=1, g_menuDrawTraceDump=0;
static unsigned g_menuTraceFrame=0,g_menuTraceOrdinal=0,g_menuTracePath=0;
struct MenuTracePath {
    unsigned previous;
    explicit MenuTracePath(unsigned mask):previous(g_menuTracePath){g_menuTracePath|=mask;}
    ~MenuTracePath(){g_menuTracePath=previous;}
};
struct MenuDrawKey {
    uintptr_t rt,depth,vs,ps,decl,vb,ib,texture,textureSurface;
    D3DVIEWPORT9 vp;
    RECT scissor;
    UINT rtW,rtH,texW,texH,texFmt,kind,topology,primitives,vertices,start,minIndex,stride,offset;
    INT base;
    DWORD blend,srcBlend,dstBlend,z,zwrite,zfunc,scissorOn;
    unsigned path,projection,flags;
};
struct MenuDrawGroup {
    MenuDrawKey key;
    unsigned count=0,frames=0,firstFrame=0,lastFrame=0,firstOrdinal=0,lastOrdinal=0;
    float firstMatrix[16]{},lastMatrix[16]{},firstXY[2]{},lastXY[2]{};
};
static MenuDrawGroup g_menuGroups[2048];
static unsigned g_menuGroupCount=0,g_menuTraceDropped=0,g_menuTraceStart=0;
static unsigned long long g_menuTraceState=~0ull;
static void MenuDrawTraceFlush() {
    if(!g_menuGroupCount)return;
    Log("MenuDrawTrace state=%llu class=%u groups=%u dropped=%u sample=frames20..27 path=1:stereo,2:hud,4:provider",
        g_menuTraceState,(unsigned)g_menuTraceState,g_menuGroupCount,g_menuTraceDropped);
    // Groups live in a hashed sparse table, not at [0, groupCount). Iterating
    // only the count emitted empty slots and lost real high-index UI groups.
    for(unsigned i=0;i<2048;++i){const auto& a=g_menuGroups[i];if(!a.count)continue;const auto& k=a.key;
        Log("MDG %u n=%u frames=%u first=%u:%u last=%u:%u rt=%p %ux%u ds=%p tex=%p surface=%p %ux%u fmt=%u vs=%p ps=%p decl=%p",
            i,a.count,a.frames,a.firstFrame,a.firstOrdinal,a.lastFrame,a.lastOrdinal,(void*)k.rt,k.rtW,k.rtH,(void*)k.depth,(void*)k.texture,(void*)k.textureSurface,k.texW,k.texH,k.texFmt,(void*)k.vs,(void*)k.ps,(void*)k.decl);
        Log("MDG %u kind=%u type=%u prim=%u verts=%u start=%u min=%u base=%d vb=%p ib=%p stride=%u offset=%u path=%u proj=%u flags=%u vp=%u,%u,%u,%u scissor=%ld,%ld,%ld,%ld/%lu blend=%lu,%lu,%lu depth=%lu,%lu,%lu",
            i,k.kind,k.topology,k.primitives,k.vertices,k.start,k.minIndex,k.base,(void*)k.vb,(void*)k.ib,k.stride,k.offset,k.path,k.projection,k.flags,k.vp.X,k.vp.Y,k.vp.Width,k.vp.Height,k.scissor.left,k.scissor.top,k.scissor.right,k.scissor.bottom,k.scissorOn,k.blend,k.srcBlend,k.dstBlend,k.z,k.zwrite,k.zfunc);
        Log("MDG %u c5first=[%.4g %.4g %.4g %.4g; %.4g %.4g %.4g %.4g; %.4g %.4g %.4g %.4g; %.4g %.4g %.4g %.4g] xyFirst=%.4g,%.4g xyLast=%.4g,%.4g",
            i,a.firstMatrix[0],a.firstMatrix[1],a.firstMatrix[2],a.firstMatrix[3],a.firstMatrix[4],a.firstMatrix[5],a.firstMatrix[6],a.firstMatrix[7],a.firstMatrix[8],a.firstMatrix[9],a.firstMatrix[10],a.firstMatrix[11],a.firstMatrix[12],a.firstMatrix[13],a.firstMatrix[14],a.firstMatrix[15],a.firstXY[0],a.firstXY[1],a.lastXY[0],a.lastXY[1]);
        Log("MDG %u c5last=[%.4g %.4g %.4g %.4g; %.4g %.4g %.4g %.4g; %.4g %.4g %.4g %.4g; %.4g %.4g %.4g %.4g]",
            i,a.lastMatrix[0],a.lastMatrix[1],a.lastMatrix[2],a.lastMatrix[3],a.lastMatrix[4],a.lastMatrix[5],a.lastMatrix[6],a.lastMatrix[7],a.lastMatrix[8],a.lastMatrix[9],a.lastMatrix[10],a.lastMatrix[11],a.lastMatrix[12],a.lastMatrix[13],a.lastMatrix[14],a.lastMatrix[15]);
    }
}
static void MenuDrawTraceState() {
    const auto token=VrMenuStateToken();
    if(token!=g_menuTraceState){
        MenuDrawTraceFlush();g_menuGroupCount=0;g_menuTraceDropped=0;
        memset(g_menuGroups,0,sizeof(g_menuGroups));g_menuTraceState=token;g_menuTraceStart=g_menuTraceFrame;
    }
    if(InterlockedExchange(&g_menuDrawTraceDump,0))MenuDrawTraceFlush();
}
static void MenuDrawTraceFrame(){if(!Rd(g_menuDrawTrace))return;++g_menuTraceFrame;g_menuTraceOrdinal=0;MenuDrawTraceState();}
static void MenuDrawTraceRecord(IDirect3DDevice9* dev,unsigned kind,D3DPRIMITIVETYPE t,
        UINT primitives,UINT vertices,UINT start,UINT minIndex,INT base,const void* data,UINT stride,bool suppressed=false) {
    if(!Rd(g_menuDrawTrace))return;
    MenuDrawTraceState();++g_menuTraceOrdinal;
    const unsigned age=g_menuTraceFrame-g_menuTraceStart;
    if(age<20||age>=28)return;
    MenuDrawKey k{};float matrix[16]{};
    k.kind=kind;k.topology=t;k.primitives=primitives;k.vertices=vertices;k.start=start;k.minIndex=minIndex;k.base=base;k.stride=stride;
    k.path=g_menuTracePath|(Rd(g_vrMenuProviderDrawing)?4:0);
    k.flags=(Rd(g_rtIsScene)?1:0)|(g_boundShaderLayout.shadow?2:0)|(Rd(g_vrMenuSourceBound)?4:0)|(suppressed?8:0);
    IDirect3DSurface9 *rt=nullptr,*ds=nullptr;
    IDirect3DVertexShader9* vs=nullptr;IDirect3DPixelShader9* ps=nullptr;
    IDirect3DVertexDeclaration9* decl=nullptr;IDirect3DVertexBuffer9* vb=nullptr;IDirect3DIndexBuffer9* ib=nullptr;
    IDirect3DBaseTexture9* tex=nullptr;
    dev->GetRenderTarget(0,&rt);dev->GetDepthStencilSurface(&ds);dev->GetVertexShader(&vs);dev->GetPixelShader(&ps);dev->GetVertexDeclaration(&decl);
    dev->GetTexture(0,&tex);dev->GetViewport(&k.vp);dev->GetScissorRect(&k.scissor);
    if(kind<2)dev->GetStreamSource(0,&vb,&k.offset,&k.stride);
    if(kind==1)dev->GetIndices(&ib);
    k.rt=(uintptr_t)rt;k.depth=(uintptr_t)ds;k.vs=(uintptr_t)vs;k.ps=(uintptr_t)ps;k.decl=(uintptr_t)decl;k.vb=(uintptr_t)vb;k.ib=(uintptr_t)ib;k.texture=(uintptr_t)tex;
    if(rt){D3DSURFACE_DESC d{};rt->GetDesc(&d);k.rtW=d.Width;k.rtH=d.Height;rt->Release();}
    if(tex && tex->GetType()==D3DRTYPE_TEXTURE){
        auto texture=(IDirect3DTexture9*)tex;D3DSURFACE_DESC d{};texture->GetLevelDesc(0,&d);k.texW=d.Width;k.texH=d.Height;k.texFmt=d.Format;
        IDirect3DSurface9* surface=nullptr;if(SUCCEEDED(texture->GetSurfaceLevel(0,&surface))&&surface){k.textureSurface=(uintptr_t)surface;surface->Release();}
    }
    if(ds)ds->Release();if(vs)vs->Release();if(ps)ps->Release();if(decl)decl->Release();if(vb)vb->Release();if(ib)ib->Release();if(tex)tex->Release();
    dev->GetRenderState(D3DRS_ALPHABLENDENABLE,&k.blend);dev->GetRenderState(D3DRS_SRCBLEND,&k.srcBlend);dev->GetRenderState(D3DRS_DESTBLEND,&k.dstBlend);
    dev->GetRenderState(D3DRS_ZENABLE,&k.z);dev->GetRenderState(D3DRS_ZWRITEENABLE,&k.zwrite);dev->GetRenderState(D3DRS_ZFUNC,&k.zfunc);dev->GetRenderState(D3DRS_SCISSORTESTENABLE,&k.scissorOn);
    if(SUCCEEDED(dev->GetVertexShaderConstantF(5,matrix,4)))k.projection=std::fabs(matrix[15]-1.f)<.001f?1:2;
    // Fixed hash table avoids a linear scan per draw. Key includes actual scissor/viewport.
    unsigned hash=2166136261u;const auto bytes=(const unsigned char*)&k;
    for(unsigned j=0;j<sizeof(k);++j)hash=(hash^bytes[j])*16777619u;
    MenuDrawGroup* group=nullptr;
    for(unsigned probe=0;probe<2048;++probe){auto& g=g_menuGroups[(hash+probe)&2047];
        if(!g.count){g.key=k;++g_menuGroupCount;g.firstFrame=g_menuTraceFrame;g.firstOrdinal=g_menuTraceOrdinal;memcpy(g.firstMatrix,matrix,sizeof(matrix));if(data&&stride>=8)memcpy(g.firstXY,data,8);group=&g;break;}
        if(!memcmp(&g.key,&k,sizeof(k))){group=&g;break;}
    }
    if(!group){++g_menuTraceDropped;return;}
    if(!group->count||group->lastFrame!=g_menuTraceFrame)++group->frames;
    ++group->count;group->lastFrame=g_menuTraceFrame;group->lastOrdinal=g_menuTraceOrdinal;memcpy(group->lastMatrix,matrix,sizeof(matrix));if(data&&stride>=8)memcpy(group->lastXY,data,8);
}
static HRESULT TraceDrawPrim(IDirect3DDevice9* d,D3DPRIMITIVETYPE t,UINT s,UINT n){MenuDrawTraceRecord(d,0,t,n,0,s,0,0,nullptr,0);return g_origDrawPrim(d,t,s,n);}
static HRESULT TraceDrawIndexedPrim(IDirect3DDevice9* d,D3DPRIMITIVETYPE t,INT b,UINT m,UINT v,UINT s,UINT n){MenuDrawTraceRecord(d,1,t,n,v,s,m,b,nullptr,0);return g_origDrawIndexedPrim(d,t,b,m,v,s,n);}
static HRESULT TraceDrawPrimUP(IDirect3DDevice9* d,D3DPRIMITIVETYPE t,UINT n,const void* v,UINT s){MenuDrawTraceRecord(d,2,t,n,0,0,0,0,v,s);return g_origDrawPrimUP(d,t,n,v,s);}
static HRESULT TraceDrawIndexedPrimUP(IDirect3DDevice9* d,D3DPRIMITIVETYPE t,UINT m,UINT v,UINT n,const void* i,D3DFORMAT f,const void* p,UINT s){MenuDrawTraceRecord(d,3,t,n,v,0,m,0,p,s);return g_origDrawIndexedPrimUP(d,t,m,v,n,i,f,p,s);}
