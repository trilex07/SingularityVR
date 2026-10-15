#pragma once
#include "hip_knife.h"
#include <d3d9.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace hipknife {
using DrawUP=HRESULT (STDMETHODCALLTYPE*)(IDirect3DDevice9*,D3DPRIMITIVETYPE,UINT,UINT,UINT,
    const void*,D3DFORMAT,const void*,UINT);
struct Calls {
    DrawUP draw=nullptr;
    HRESULT (STDMETHODCALLTYPE* target)(IDirect3DDevice9*,DWORD,IDirect3DSurface9*)=nullptr;
    HRESULT (STDMETHODCALLTYPE* vs)(IDirect3DDevice9*,IDirect3DVertexShader9*)=nullptr;
    HRESULT (STDMETHODCALLTYPE* ps)(IDirect3DDevice9*,IDirect3DPixelShader9*)=nullptr;
    HRESULT (STDMETHODCALLTYPE* texture)(IDirect3DDevice9*,DWORD,IDirect3DBaseTexture9*)=nullptr;
    HRESULT (STDMETHODCALLTYPE* clear)(IDirect3DDevice9*,DWORD,const D3DRECT*,DWORD,D3DCOLOR,float,DWORD)=nullptr;
    HRESULT (STDMETHODCALLTYPE* viewport)(IDirect3DDevice9*,const D3DVIEWPORT9*)=nullptr;
    bool Valid()const{return draw&&target&&vs&&ps&&texture&&clear&&viewport;}
};
template<class T> inline void Release(T*& p){if(p){p->Release();p=nullptr;}}
// ALL state restores stream0/indexbuffer cleared by DrawIndexedPrimitiveUP,
// but does NOT save RT/MRT/depth. Raw driver calls leave hook CPU caches alone.
template<class Dev=IDirect3DDevice9,class Block=IDirect3DStateBlock9,
         class Surf=IDirect3DSurface9,class Api=Calls>
struct StateGuard {
    Dev* dev;Block* block;const Api& calls;
    Surf* rt[4]{};Surf* ds=nullptr;
    D3DVIEWPORT9 vp{};unsigned count=0;bool valid=false;
    StateGuard(Dev* d,Block* b,const Api& c):dev(d),block(b),calls(c){
        D3DCAPS9 caps{};
        if(FAILED(d->GetDeviceCaps(&caps))||FAILED(d->GetViewport(&vp))||FAILED(b->Capture()))return;
        count=std::min(4u,unsigned(caps.NumSimultaneousRTs));if(!count)return;
        for(unsigned i=0;i<count;++i){
            const HRESULT hr=d->GetRenderTarget(i,&rt[i]);
            if(FAILED(hr)&&hr!=D3DERR_NOTFOUND)return;
        }
        if(!rt[0])return;
        const HRESULT hr=d->GetDepthStencilSurface(&ds);
        if(FAILED(hr)&&hr!=D3DERR_NOTFOUND)return;
        valid=true;
    }
    bool Restore(){
        if(!valid)return false;
        valid=false;bool ok=SUCCEEDED(dev->SetDepthStencilSurface(nullptr));
        for(unsigned i=1;i<count;++i)ok=SUCCEEDED(calls.target(dev,i,nullptr))&&ok;
        ok=SUCCEEDED(calls.target(dev,0,rt[0]))&&ok;
        for(unsigned i=1;i<count;++i)ok=SUCCEEDED(calls.target(dev,i,rt[i]))&&ok;
        ok=SUCCEEDED(dev->SetDepthStencilSurface(ds))&&ok;
        ok=SUCCEEDED(block->Apply())&&ok;ok=SUCCEEDED(calls.viewport(dev,&vp))&&ok;
        return ok;
    }
    ~StateGuard(){if(valid)Restore();for(auto& p:rt)Release(p);Release(ds);}
};
struct Vertex {float x,y,z;DWORD colour;float u,v;};
static_assert(sizeof(Vertex)==24,"SVRK v2 position/white diffuse/native UV");
inline bool Decode(const unsigned char* b,size_t size,std::vector<Vertex>& v,std::vector<WORD>& ix){
    if(!b||size<16||std::memcmp(b,"SVRK",4))return false;
    unsigned version,n,indices;
    std::memcpy(&version,b+4,4);std::memcpy(&n,b+8,4);std::memcpy(&indices,b+12,4);
    if(version!=2||!n||n>65535||!indices||indices>196605||indices%3||
       size!=16+size_t(n)*sizeof(Vertex)+size_t(indices)*2)return false;
    std::vector<Vertex> staged(n);std::vector<WORD> stagedIx(indices);
    std::memcpy(staged.data(),b+16,n*sizeof(Vertex));
    std::memcpy(stagedIx.data(),b+16+n*sizeof(Vertex),indices*2);
    for(const auto& p:staged)if(!Finite({p.x,p.y,p.z})||Length({p.x,p.y,p.z})>1.f||
        !std::isfinite(p.u)||!std::isfinite(p.v)||std::fabs(p.u)>16||std::fabs(p.v)>16||p.colour!=0xffffffff)return false;
    for(WORD i:stagedIx)if(i>=n)return false;
    v=std::move(staged);ix=std::move(stagedIx);return true;
}
inline bool Read(const std::string& path,std::vector<unsigned char>& data,std::size_t limit){
    FILE* f=nullptr;if(fopen_s(&f,path.c_str(),"rb")||!f)return false;
    bool ok=false;if(!std::fseek(f,0,SEEK_END)){const long size=std::ftell(f);
        if(size>0&&std::size_t(size)<=limit&&!std::fseek(f,0,SEEK_SET)){
            data.resize(size);ok=std::fread(data.data(),1,size,f)==std::size_t(size);}}
    std::fclose(f);return ok;
}
inline bool DecodeTexture(const unsigned char* p,std::size_t bytes,UINT& width,UINT& height,
                          std::vector<unsigned char>& texels){
    if(!p||bytes<16||std::memcmp(p,"SVRT",4))return false;
    unsigned fields[3];std::memcpy(fields,p+4,12);
    const auto w=fields[1],h=fields[2];
    if(fields[0]!=1||!w||!h||w>2048||h>2048||bytes!=16+std::size_t(w)*h*4)return false;
    std::vector<unsigned char> staged(p+16,p+bytes);texels=std::move(staged);width=w;height=h;return true;
}
// D3D9Ex-compatible upload: lock SYSTEMMEM only, UpdateTexture into DEFAULT.
// No MANAGED pool, direct DEFAULT LockRect, bind changes or persistent staging.
template<class Dev,class Tex>
inline bool UploadAlbedo(Dev* dev,UINT w,UINT h,const std::vector<unsigned char>& texels,Tex*& gpu){
    if(gpu||!w||!h||w>2048||h>2048||texels.size()!=std::size_t(w)*h*4)return false;
    Tex* staging=nullptr;
    if(FAILED(dev->CreateTexture(w,h,1,0,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&staging,nullptr))||!staging){Release(staging);return false;}
    D3DLOCKED_RECT rect{};
    bool ok=SUCCEEDED(staging->LockRect(0,&rect,nullptr,0));
    if(ok){
        ok=rect.pBits&&rect.Pitch>=int(w*4);
        if(ok)for(unsigned y=0;y<h;++y)
            std::memcpy(static_cast<unsigned char*>(rect.pBits)+std::size_t(y)*rect.Pitch,
                        texels.data()+std::size_t(y)*w*4,w*4);
        ok=SUCCEEDED(staging->UnlockRect(0))&&ok;
    }
    if(ok)ok=SUCCEEDED(dev->CreateTexture(w,h,1,0,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&gpu,nullptr))&&gpu;
    if(ok)ok=SUCCEEDED(dev->UpdateTexture(staging,gpu));
    Release(staging);if(!ok)Release(gpu);return ok;
}
class Renderer {
    std::vector<Vertex> source,scratch;std::vector<WORD> indices;
    IDirect3DSurface9* depth=nullptr;IDirect3DStateBlock9* state=nullptr;
    IDirect3DTexture9* texture=nullptr;std::vector<unsigned char> texels;
    UINT textureWidth=0,textureHeight=0;
    UINT width=0,height=0;bool failed=false;
public:
    bool Load(const std::string& file){
        ReleaseGPU();source.clear();scratch.clear();indices.clear();texels.clear();textureWidth=textureHeight=0;
        std::vector<unsigned char> data;std::vector<Vertex> vertices;std::vector<WORD> ix;
        if(!Read(file,data,2*1024*1024)||!Decode(data.data(),data.size(),vertices,ix))return false;
        const auto slash=file.find_last_of("\\/");
        const auto path=(slash==std::string::npos?std::string():file.substr(0,slash+1))+"knife.svrt";
        if(!Read(path,data,16+2048*2048*4)||!DecodeTexture(data.data(),data.size(),textureWidth,textureHeight,texels))return false;
        source=std::move(vertices);indices=std::move(ix);scratch.resize(source.size());return true;
    }
    unsigned Vertices()const{return unsigned(source.size());}
    unsigned Triangles()const{return unsigned(indices.size()/3);}
    unsigned TextureWidth()const{return textureWidth;}
    unsigned TextureHeight()const{return textureHeight;}
    void ReleaseGPU(){Release(depth);Release(state);Release(texture);width=height=0;failed=false;}
    bool Ensure(IDirect3DDevice9* dev,UINT w,UINT h){
        if(source.empty()||texels.empty()||failed||w<2||w%2||h<2)return false;
        if(state&&w==width&&h==height)return true;
        ReleaseGPU();
        if(FAILED(dev->CreateDepthStencilSurface(w,h,D3DFMT_D16,D3DMULTISAMPLE_NONE,0,TRUE,&depth,nullptr))||
           FAILED(dev->CreateStateBlock(D3DSBT_ALL,&state))||!UploadAlbedo(dev,textureWidth,textureHeight,texels,texture)){ReleaseGPU();failed=true;return false;}
        width=w;height=h;return true;
    }
    bool Draw(IDirect3DDevice9* dev,const Calls& c,IDirect3DSurface9* back,
              const Pose& knife,const Pose (&eyes)[2],float tx,float ty){
        D3DMATRIX proj{},ident{};
        if(!state||!texture||!c.Valid()||!back||!Valid(knife.q)||!Finite(knife.p)||
           !Projection(tx,ty,proj.m))return false;
        for(const auto& eye:eyes)if(!Valid(eye.q)||!Finite(eye.p))return false;
        for(int i=0;i<4;++i)ident.m[i][i]=1;
        StateGuard<> guard(dev,state,c);if(!guard.valid)return false;
        if(FAILED(dev->SetDepthStencilSurface(nullptr)))return false;
        for(unsigned i=1;i<guard.count;++i)if(FAILED(c.target(dev,i,nullptr)))return false;
        if(FAILED(c.target(dev,0,back))||FAILED(dev->SetDepthStencilSurface(depth))||
           FAILED(c.clear(dev,0,nullptr,D3DCLEAR_ZBUFFER,0,1,0))||
           FAILED(c.vs(dev,nullptr))||FAILED(c.ps(dev,nullptr))||FAILED(c.texture(dev,0,texture))||
           FAILED(dev->SetFVF(D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1)))return false;
        struct RS{D3DRENDERSTATETYPE key;DWORD value;};
        const RS states[]={{D3DRS_ZENABLE,D3DZB_TRUE},{D3DRS_ZWRITEENABLE,TRUE},{D3DRS_ZFUNC,D3DCMP_LESSEQUAL},
            {D3DRS_ALPHABLENDENABLE,FALSE},{D3DRS_SEPARATEALPHABLENDENABLE,FALSE},{D3DRS_ALPHATESTENABLE,FALSE},
            {D3DRS_CULLMODE,D3DCULL_NONE},{D3DRS_LIGHTING,FALSE},{D3DRS_FOGENABLE,FALSE},
            {D3DRS_STENCILENABLE,FALSE},{D3DRS_SCISSORTESTENABLE,FALSE},{D3DRS_CLIPPING,TRUE},
            {D3DRS_VERTEXBLEND,D3DVBF_DISABLE},{D3DRS_INDEXEDVERTEXBLENDENABLE,FALSE},
            {D3DRS_CLIPPLANEENABLE,0},{D3DRS_COLORWRITEENABLE,15},{D3DRS_SRGBWRITEENABLE,FALSE},
            {D3DRS_FILLMODE,D3DFILL_SOLID},{D3DRS_DEPTHBIAS,0},{D3DRS_SLOPESCALEDEPTHBIAS,0}};
        for(const auto& r:states)if(FAILED(dev->SetRenderState(r.key,r.value)))return false;
        if(FAILED(dev->SetTransform(D3DTS_WORLD,&ident))||FAILED(dev->SetTransform(D3DTS_VIEW,&ident))||
           FAILED(dev->SetTransform(D3DTS_PROJECTION,&proj)))return false;
        if(FAILED(dev->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1))||
           FAILED(dev->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE))||
           FAILED(dev->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1))||
           FAILED(dev->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_DIFFUSE))||
           FAILED(dev->SetTextureStageState(0,D3DTSS_RESULTARG,D3DTA_CURRENT))||
           FAILED(dev->SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,0))||
           FAILED(dev->SetTextureStageState(0,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_DISABLE))||
           FAILED(dev->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE)))return false;
        const struct Sampler {D3DSAMPLERSTATETYPE key;DWORD value;} sampler[]={
            {D3DSAMP_MINFILTER,D3DTEXF_LINEAR},{D3DSAMP_MAGFILTER,D3DTEXF_LINEAR},
            {D3DSAMP_MIPFILTER,D3DTEXF_NONE},{D3DSAMP_ADDRESSU,D3DTADDRESS_WRAP},
            {D3DSAMP_ADDRESSV,D3DTADDRESS_WRAP},{D3DSAMP_SRGBTEXTURE,FALSE},
            {D3DSAMP_MAXMIPLEVEL,0},{D3DSAMP_MIPMAPLODBIAS,0}};
        for(const auto& s:sampler)if(FAILED(dev->SetSamplerState(0,s.key,s.value)))return false;
        for(unsigned e=0;e<2;++e){
            const D3DVIEWPORT9 vp{e*width/2,0,width/2,height,0,1};
            if(FAILED(c.viewport(dev,&vp)))return false;
            for(size_t i=0;i<source.size();++i){
                const auto& v=source[i];const V p=EyeLocal(knife.p+Rotate(knife.q,{v.x,v.y,v.z}),eyes[e]);
                scratch[i]={p.x,p.y,p.z,0xffffffff,v.u,v.v};
            }
            if(FAILED(c.draw(dev,D3DPT_TRIANGLELIST,0,UINT(scratch.size()),UINT(indices.size()/3),
                            indices.data(),D3DFMT_INDEX16,scratch.data(),sizeof(Vertex))))return false;
        }
        return guard.Restore();
    }
};
} // namespace hipknife
