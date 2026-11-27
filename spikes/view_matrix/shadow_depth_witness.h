#pragma once
#include <cstdint>
#include <cstdio>

namespace tmddepthwitness {
using DrawUP = HRESULT (STDMETHODCALLTYPE*)(IDirect3DDevice9*,D3DPRIMITIVETYPE,UINT,const void*,UINT);
using SetVS = HRESULT (STDMETHODCALLTYPE*)(IDirect3DDevice9*,IDirect3DVertexShader9*);
using SetPS = HRESULT (STDMETHODCALLTYPE*)(IDirect3DDevice9*,IDirect3DPixelShader9*);
struct Calls { DrawUP draw; SetVS vs; SetPS ps; };
struct Vertex { float x,y,z,rhw; DWORD color; };

constexpr float kDepths[]={0.75f,0.90f,0.95f,0.97f,0.98f,0.99f,0.997f,1.0f};
constexpr unsigned kBins=sizeof(kDepths)/sizeof(kDepths[0]), kBudget=16;
struct Observation {
    unsigned draw=0, op=0, group=0, point=0, after=0;
    DWORD alphaTest=0, alphaFunc=0, alphaRef=0, bias=0, slopeBias=0;
    HRESULT capture=E_FAIL, restore=E_FAIL;
    HRESULT gpu[kBins]{};
};
static Observation observations[kBudget]{};
static IDirect3DQuery9* queries[kBudget][kBins]{};
static IDirect3DStateBlock9* state=nullptr;
static unsigned count=0, overflow=0;
static bool requested=false, active=false, failed=false;
static Calls calls{};
static HRESULT setup=E_FAIL;

inline void Release() {
    for(auto& row:queries)for(auto& q:row){if(q)q->Release();q=nullptr;}
    if(state)state->Release();
    state=nullptr;
    count=overflow=0;
    requested=active=failed=false;
    setup=E_FAIL;
}

inline void Arm(IDirect3DDevice9* dev,bool enabled,Calls native) {
    requested=enabled;
    if(!enabled||!native.draw||!native.vs||!native.ps)return;
    setup=dev->CreateStateBlock(D3DSBT_ALL,&state);
    if(FAILED(setup))return;
    for(auto& row:queries)for(auto& query:row){
        setup=dev->CreateQuery(D3DQUERYTYPE_OCCLUSION,&query);
        if(FAILED(setup))return;
    }
    calls=native;
    active=true;
}

template<class Shader> inline bool KnownCode(Shader* shader,UINT expected,uint32_t fingerprint) {
    if(!shader)return false;
    UINT bytes=0;
    if(FAILED(shader->GetFunction(nullptr,&bytes))||bytes!=expected)return false;
    uint8_t code[2736];
    if(FAILED(shader->GetFunction(code,&bytes))||bytes!=expected)return false;
    uint32_t hash=2166136261u;
    for(UINT i=0;i<bytes;++i)hash=(hash^code[i])*16777619u;
    return hash==fingerprint;
}

inline bool Match(IDirect3DDevice9* dev) {
    IDirect3DPixelShader9* ps=nullptr;
    IDirect3DVertexShader9* vs=nullptr;
    const bool checked=SUCCEEDED(dev->GetPixelShader(&ps))&&KnownCode(ps,2736,0xBF50BD06u)&&
        SUCCEEDED(dev->GetVertexShader(&vs))&&KnownCode(vs,1116,0xBFAB5B3Eu);
    if(ps)ps->Release();
    if(vs)vs->Release();
    return checked;
}

inline void Sample(IDirect3DDevice9* dev,unsigned draw,unsigned op,unsigned group,
                   unsigned point,unsigned after,unsigned x,unsigned y) {
    if(count==kBudget){++overflow;return;}
    const unsigned slot=count++;
    auto& row=observations[slot];
    row={};
    row.draw=draw;row.op=op;row.group=group;row.point=point;row.after=after;
    for(auto& result:row.gpu)result=E_FAIL;

    dev->GetRenderState(D3DRS_ALPHATESTENABLE,&row.alphaTest);
    dev->GetRenderState(D3DRS_ALPHAFUNC,&row.alphaFunc);
    dev->GetRenderState(D3DRS_ALPHAREF,&row.alphaRef);
    dev->GetRenderState(D3DRS_DEPTHBIAS,&row.bias);
    dev->GetRenderState(D3DRS_SLOPESCALEDEPTHBIAS,&row.slopeBias);
    row.capture=state->Capture();
    if(FAILED(row.capture)){failed=true;return;}
    bool clean=SUCCEEDED(calls.vs(dev,nullptr))&&SUCCEEDED(calls.ps(dev,nullptr))&&
        SUCCEEDED(dev->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE));
    constexpr D3DRENDERSTATETYPE states[]={D3DRS_ZENABLE,D3DRS_ZWRITEENABLE,D3DRS_ZFUNC,
        D3DRS_COLORWRITEENABLE,D3DRS_ALPHATESTENABLE,D3DRS_STENCILENABLE,D3DRS_CULLMODE,
        D3DRS_DEPTHBIAS,D3DRS_SLOPESCALEDEPTHBIAS,D3DRS_CLIPPLANEENABLE};
    constexpr DWORD values[]={D3DZB_TRUE,FALSE,D3DCMP_LESSEQUAL,0,FALSE,FALSE,D3DCULL_NONE,0,0,0};
    for(unsigned i=0;i<sizeof(states)/sizeof(states[0]);++i)
        clean=SUCCEEDED(dev->SetRenderState(states[i],values[i]))&&clean;
    if(clean){
        const float left=float(x)-.5f, right=float(x)+.5f, top=float(y)-.5f, bottom=float(y)+.5f;
        for(unsigned i=0;i<kBins;++i){
            auto* q=queries[slot][i];
            row.gpu[i]=q->Issue(D3DISSUE_BEGIN);
            if(FAILED(row.gpu[i]))continue;
            const float z=kDepths[i];
            const Vertex quad[]={{left,top,z,1,0xffffffff}, {right,top,z,1,0xffffffff},
                                 {left,bottom,z,1,0xffffffff}, {right,bottom,z,1,0xffffffff}};
            const auto drawHr=calls.draw(dev,D3DPT_TRIANGLESTRIP,2,quad,sizeof(Vertex));
            const auto endHr=q->Issue(D3DISSUE_END);
            row.gpu[i]=FAILED(drawHr)?drawHr:endHr;
        }
    }
    // The capture does not touch RT or DS. Restore original shaders, state and
    // stream inputs before forwarding the game's next draw.
    row.restore=state->Apply();
    failed=!clean||FAILED(row.restore);
}

inline void Dump(FILE* file) {
    if(!requested)return;
    fprintf(file,"tmd-depth-query active=%d setup=%08lX rows=%u overflow=%u restoreError=%d\n",
            active,setup,count,overflow,failed);
    for(unsigned i=0;i<count;++i){
        const auto& row=observations[i];
        fprintf(file,"tmd-depth draw=%u op=%u group=%u point=%u after=%u capture=%08lX restore=%08lX "
            "alphaTest=%lu alphaFunc=%lu alphaRef=%lu bias=%08lX slope=%08lX",
            row.draw,row.op,row.group,row.point,row.after,row.capture,row.restore,
            row.alphaTest,row.alphaFunc,row.alphaRef,row.bias,row.slopeBias);
        for(unsigned j=0;j<kBins;++j){
            DWORD pixels=0;
            auto hr=row.gpu[j];
            if(SUCCEEDED(hr))hr=queries[i][j]->GetData(&pixels,sizeof(pixels),0);
            fprintf(file," z%.5f=%08lX/%lu",kDepths[j],hr,pixels);
        }
        fputc('\n',file);
    }
}
}
