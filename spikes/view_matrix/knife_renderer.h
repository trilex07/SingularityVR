#pragma once
#include "hip_knife.h"
#include "knife_hand_grip.h"
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
    HRESULT (STDMETHODCALLTYPE* vsConstants)(IDirect3DDevice9*,UINT,const float*,UINT)=nullptr;
    bool Valid()const{return draw&&target&&vs&&ps&&texture&&clear&&viewport;}
};
template<class T> inline void Release(T*& p){if(p){p->Release();p=nullptr;}}
template<class Dev,class Surf,class Api>
inline bool PrimePrivateDepth(Dev* dev,Surf* privateDepth,Surf* originalDepth,const Api& api){
    if(!privateDepth||privateDepth==originalDepth)return false;
    return SUCCEEDED(dev->SetDepthStencilSurface(privateDepth))&&
        SUCCEEDED(api.clear(dev,0,nullptr,D3DCLEAR_ZBUFFER,0,1.f,0));
}
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
} // hipknife: the optional material uses the same tested upload contract.
#include "knife_scene_lighting.h"
namespace hipknife {
class Renderer {
    std::vector<Vertex> source,scratch;std::vector<WORD> indices;
    std::vector<knifehand::ScreenVertex> jointScratch;std::vector<WORD> jointIndices;
    IDirect3DSurface9* depth=nullptr;IDirect3DStateBlock9* state=nullptr;
    IDirect3DTexture9* texture=nullptr;std::vector<unsigned char> texels;
    UINT textureWidth=0,textureHeight=0;
    UINT width=0,height=0;bool failed=false;
    knifelight::Renderer lighting;bool sceneLighting=false;
    V sourceCentre{};
public:
    bool Load(const std::string& file,bool sceneLit=false){
        const auto slash=file.find_last_of("\\/");
        return LoadWithAlbedo(file,(slash==std::string::npos?std::string():file.substr(0,slash+1))+"knife.svrt",sceneLit);
    }
    // Explicit asset pair for other measured rigid meshes. The original knife
    // entry point above retains its exact path and rendering/lighting behavior.
    bool LoadWithAlbedo(const std::string& file,const std::string& albedo,bool sceneLit=false){
        ReleaseGPU();source.clear();scratch.clear();indices.clear();texels.clear();textureWidth=textureHeight=0;
        std::vector<unsigned char> data;std::vector<Vertex> vertices;std::vector<WORD> ix;
        if(!Read(file,data,2*1024*1024)||!Decode(data.data(),data.size(),vertices,ix))return false;
        const auto slash=file.find_last_of("\\/");
        if(!Read(albedo,data,16+2048*2048*4)||!DecodeTexture(data.data(),data.size(),textureWidth,textureHeight,texels))return false;
        source=std::move(vertices);indices=std::move(ix);scratch.resize(source.size());
        V lo{1,1,1},hi{-1,-1,-1};for(const auto& v:source){
            lo.x=std::min(lo.x,v.x);lo.y=std::min(lo.y,v.y);lo.z=std::min(lo.z,v.z);
            hi.x=std::max(hi.x,v.x);hi.y=std::max(hi.y,v.y);hi.z=std::max(hi.z,v.z);}
        sourceCentre=(lo+hi)*.5f;
        sceneLighting=sceneLit;
        if(sceneLighting&&!lighting.Load(slash==std::string::npos?std::string():file.substr(0,slash+1)))return false;
        if(Triangles()<=5000){jointScratch.resize(Triangles()*12);jointIndices.resize(jointScratch.size());
            for(unsigned i=0;i<jointIndices.size();++i)jointIndices[i]=(WORD)i;}
        return true;
    }
    unsigned Vertices()const{return unsigned(source.size());}
    unsigned Triangles()const{return unsigned(indices.size()/3);}
    unsigned TextureWidth()const{return textureWidth;}
    unsigned TextureHeight()const{return textureHeight;}
    V SourceCentre()const{return sourceCentre;}
    void ReleaseGPU(){lighting.Release();Release(depth);Release(state);Release(texture);width=height=0;failed=false;}
    bool Ensure(IDirect3DDevice9* dev,UINT w,UINT h){
        if(source.empty()||texels.empty()||failed||w<2||w%2||h<2)return false;
        if(state&&w==width&&h==height)return true;
        ReleaseGPU();
        if(FAILED(dev->CreateDepthStencilSurface(w,h,D3DFMT_D16,D3DMULTISAMPLE_NONE,0,TRUE,&depth,nullptr))||
           FAILED(dev->CreateStateBlock(D3DSBT_ALL,&state))||!UploadAlbedo(dev,textureWidth,textureHeight,texels,texture)){ReleaseGPU();failed=true;return false;}
        width=w;height=h;return true;
    }
    bool EnsureShared(IDirect3DDevice9* dev,UINT w,UINT h){
        if(source.size()!=662||Triangles()!=1010||jointScratch.empty()||failed)return false;
        return Ensure(dev,w,h)&&(!sceneLighting||lighting.Ensure(dev));
    }
    bool LightingMatch(IDirect3DDevice9* dev){return sceneLighting&&lighting.Match(dev);}
    bool DrawLitShared(IDirect3DDevice9* dev,const Calls& c,const float world[4][4],const float clip[4][4]){
        return lighting.Draw(dev,c,texture,world,clip);
    }
    IDirect3DStateBlock9* SharedState()const{return state;}
    IDirect3DSurface9* SharedDepth()const{return depth;}
    // Called INSIDE the original hand StereoPair, with that eye's real camera,
    // viewport/scissor. Caller has installed a PRIVATE depth shared with an
    // original-native glove depth prepass, not the world's/native depth buffer.
    bool DrawShared(IDirect3DDevice9* dev,const Calls& c,const float componentToClip[4][4]){
        if(!state||!texture||!c.Valid()||jointScratch.empty())return false;
        D3DVIEWPORT9 v{};if(FAILED(dev->GetViewport(&v)))return false;
        const knifehand::Viewport vp{(float)v.X,(float)v.Y,(float)v.Width,(float)v.Height,v.MinZ,v.MaxZ};
        unsigned count=0;
        for(unsigned i=0;i<indices.size();i+=3){
            knifehand::ClipVertex input[3],clipped[12];
            for(unsigned c0=0;c0<3;++c0){const auto& p=source[indices[i+c0]];
                if(!knifehand::Transform(input[c0],{p.x,p.y,p.z},p.u,p.v,componentToClip))return false;}
            const unsigned n=knifehand::ClipTriangle(input,clipped);
            if(count+n>jointScratch.size())return false;
            for(unsigned j=0;j<n;++j)if(!knifehand::Screen(clipped[j],vp,jointScratch[count+j]))return false;
            count+=n;
        }
        if(!count)return true; // entirely outside near/far: no draw, not a GPU error.
        if(FAILED(c.vs(dev,nullptr))||FAILED(c.ps(dev,nullptr))||FAILED(c.texture(dev,0,texture))||
           FAILED(dev->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1)))return false;
        const struct RS{D3DRENDERSTATETYPE key;DWORD value;} rs[]={
            {D3DRS_ZENABLE,D3DZB_TRUE},{D3DRS_ZWRITEENABLE,TRUE},{D3DRS_ZFUNC,D3DCMP_LESSEQUAL},
            {D3DRS_ALPHABLENDENABLE,FALSE},{D3DRS_SEPARATEALPHABLENDENABLE,FALSE},
            {D3DRS_ALPHATESTENABLE,FALSE},{D3DRS_CULLMODE,D3DCULL_NONE},{D3DRS_LIGHTING,FALSE},
            {D3DRS_FOGENABLE,FALSE},{D3DRS_STENCILENABLE,FALSE},{D3DRS_CLIPPLANEENABLE,0},
            {D3DRS_SRGBWRITEENABLE,FALSE},{D3DRS_COLORWRITEENABLE,15},{D3DRS_FILLMODE,D3DFILL_SOLID},
            {D3DRS_VERTEXBLEND,D3DVBF_DISABLE},{D3DRS_INDEXEDVERTEXBLENDENABLE,FALSE}};
        // Preserve viewport/scissor/RT, private shared DS, depth range and bias.
        for(const auto& x:rs)if(FAILED(dev->SetRenderState(x.key,x.value)))return false;
        const struct TS{D3DTEXTURESTAGESTATETYPE key;DWORD value;} ts[]={
            {D3DTSS_COLOROP,D3DTOP_SELECTARG1},{D3DTSS_COLORARG1,D3DTA_TEXTURE},
            {D3DTSS_ALPHAOP,D3DTOP_SELECTARG1},{D3DTSS_ALPHAARG1,D3DTA_DIFFUSE},
            {D3DTSS_RESULTARG,D3DTA_CURRENT},{D3DTSS_TEXCOORDINDEX,0},
            {D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_DISABLE}};
        for(const auto& x:ts)if(FAILED(dev->SetTextureStageState(0,x.key,x.value)))return false;
        if(FAILED(dev->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE)))return false;
        const struct SS{D3DSAMPLERSTATETYPE key;DWORD value;} ss[]={
            {D3DSAMP_MINFILTER,D3DTEXF_LINEAR},{D3DSAMP_MAGFILTER,D3DTEXF_LINEAR},
            {D3DSAMP_MIPFILTER,D3DTEXF_NONE},{D3DSAMP_ADDRESSU,D3DTADDRESS_WRAP},
            {D3DSAMP_ADDRESSV,D3DTADDRESS_WRAP},{D3DSAMP_SRGBTEXTURE,FALSE},
            {D3DSAMP_MAXMIPLEVEL,0},{D3DSAMP_MIPMAPLODBIAS,0}};
        for(const auto& x:ss)if(FAILED(dev->SetSamplerState(0,x.key,x.value)))return false;
        return SUCCEEDED(c.draw(dev,D3DPT_TRIANGLELIST,0,count,count/3,jointIndices.data(),D3DFMT_INDEX16,jointScratch.data(),sizeof(jointScratch[0])));
    }
    bool Draw(IDirect3DDevice9* dev,const Calls& c,IDirect3DSurface9* back,
              const Pose& knife,const Pose (&eyes)[2],float tx,float ty,const float (*affine)[4]=nullptr){
        D3DMATRIX proj{},ident{};
        if(!state||!texture||!c.Valid()||!back||!Valid(knife.q)||!Finite(knife.p)||
           !Projection(tx,ty,proj.m))return false;
        for(const auto& eye:eyes)if(!Valid(eye.q)||!Finite(eye.p))return false;
        if(affine){for(unsigned r=0;r<4;++r)for(unsigned k=0;k<4;++k)if(!std::isfinite(affine[r][k]))return false;
            if(affine[0][3]||affine[1][3]||affine[2][3]||affine[3][3]!=1)return false;}
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
                const auto& v=source[i];
                const V world=affine?V{
                    v.x*affine[0][0]+v.y*affine[1][0]+v.z*affine[2][0]+affine[3][0],
                    v.x*affine[0][1]+v.y*affine[1][1]+v.z*affine[2][1]+affine[3][1],
                    v.x*affine[0][2]+v.y*affine[1][2]+v.z*affine[2][2]+affine[3][2]}:
                    knife.p+Rotate(knife.q,{v.x,v.y,v.z});
                const V p=EyeLocal(world,eyes[e]);
                scratch[i]={p.x,p.y,p.z,0xffffffff,v.u,v.v};
            }
            if(FAILED(c.draw(dev,D3DPT_TRIANGLELIST,0,UINT(scratch.size()),UINT(indices.size()/3),
                            indices.data(),D3DFMT_INDEX16,scratch.data(),sizeof(Vertex))))return false;
        }
        return guard.Restore();
    }
};
} // namespace hipknife
