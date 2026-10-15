#pragma once
// Opt-in renderer of offline-baked GLB morphs. No hooks, engine writes, shaders,
// D3DPOOL_MANAGED resources, asynchronous COM, or per-frame disk/heap operations.
#include "glove_animation.h"
#include <cstdio>
#include <string>
#include <d3d9.h>

namespace gloves {
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
    bool Valid() const{return draw&&target&&vs&&ps&&texture&&clear&&viewport;}
};
inline bool Read(const std::string& path,std::vector<unsigned char>& out,std::size_t limit){
    FILE* f=nullptr;if(fopen_s(&f,path.c_str(),"rb")||!f)return false;
    if(std::fseek(f,0,SEEK_END)){std::fclose(f);return false;}
    const long size=std::ftell(f);
    if(size<=0||std::size_t(size)>limit||std::fseek(f,0,SEEK_SET)){std::fclose(f);return false;}
    out.resize(size);const bool ok=std::fread(out.data(),1,size,f)==std::size_t(size);
    std::fclose(f);return ok;
}
template<class T> inline void Release(T*& p){if(p){p->Release();p=nullptr;}}

// D3DSBT_ALL does not save RT/DS. UP draws also clear stream 0/index buffer;
// the captured ALL block restores those, declarations, textures and transforms.
template<class Dev=IDirect3DDevice9,class Block=IDirect3DStateBlock9,
         class Surf=IDirect3DSurface9,class Api=Calls>
struct StateGuard {
    Dev* dev;Block* block;const Api& calls;
    Surf* rt[4]{};Surf* ds=nullptr;
    D3DVIEWPORT9 vp{};unsigned count=0;bool valid=false;
    StateGuard(Dev* d,Block* s,const Api& c):dev(d),block(s),calls(c){
        D3DCAPS9 caps{};
        if(FAILED(d->GetDeviceCaps(&caps))||FAILED(d->GetViewport(&vp))||FAILED(s->Capture()))return;
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
    ~StateGuard(){
        if(valid)Restore();
        for(auto& p:rt)Release(p);Release(ds);
    }
};

class Renderer {
    Mesh mesh[2];std::vector<unsigned char> texels;
    UINT texW=0,texH=0,width=0,height=0;
    IDirect3DTexture9* texture=nullptr;
    IDirect3DSurface9* depth=nullptr;
    IDirect3DStateBlock9* state=nullptr;
    bool cpu=false,gpuFailed=false;
public:
    unsigned draws=0;double loadMs=0; // one bounded diagnostic at startup
    bool Load(const std::string& directory){
        ReleaseGPU();cpu=false;std::vector<unsigned char> bytes;Mesh staged[2];
        for(unsigned i=0;i<2;++i){
            if(!Read(directory+(i?"\\glove_right.svrg":"\\glove_left.svrg"),bytes,2*1024*1024)||
               !Decode(bytes.data(),bytes.size(),staged[i]))return false;
        }
        if(!Read(directory+"\\glove.svrt",bytes,16*1024*1024+16)||bytes.size()<16||
           std::memcmp(bytes.data(),"SVRT",4)||U32(bytes.data()+4)!=1)return false;
        const auto w=U32(bytes.data()+8),h=U32(bytes.data()+12);
        if(!w||!h||w>2048||h>2048||bytes.size()!=16+std::size_t(w)*h*4)return false;
        texels.assign(bytes.begin()+16,bytes.end());texW=w;texH=h;
        mesh[0]=std::move(staged[0]);mesh[1]=std::move(staged[1]);cpu=true;return true;
    }
    void ReleaseGPU(){Release(texture);Release(depth);Release(state);width=height=0;gpuFailed=false;}
    bool Ready() const{return cpu&&texture&&depth&&state&&!gpuFailed;}
    unsigned Vertices(unsigned hand) const{return hand<2?unsigned(mesh[hand].source.size()):0;}
    unsigned Triangles(unsigned hand) const{return hand<2?unsigned(mesh[hand].indices.size()/3):0;}
    bool Ensure(IDirect3DDevice9* dev,UINT w,UINT h){
        if(!cpu||gpuFailed||w<2||h<2||w%2)return false;
        if(Ready()&&w==width&&h==height)return true;
        ReleaseGPU();
        const auto fail=[&](){ReleaseGPU();gpuFailed=true;return false;};
        if(FAILED(dev->CreateTexture(texW,texH,1,D3DUSAGE_DYNAMIC,D3DFMT_A8R8G8B8,
                                    D3DPOOL_DEFAULT,&texture,nullptr)))return fail();
        D3DLOCKED_RECT r{};
        if(FAILED(texture->LockRect(0,&r,nullptr,D3DLOCK_DISCARD)))return fail();
        const bool pitchOk=r.pBits&&r.Pitch>=int(texW*4);
        if(pitchOk)for(UINT y=0;y<texH;++y)std::memcpy(static_cast<unsigned char*>(r.pBits)+y*r.Pitch,
            texels.data()+std::size_t(y)*texW*4,texW*4);
        const HRESULT unlock=texture->UnlockRect(0);
        if(!pitchOk||FAILED(unlock))return fail();
        if(FAILED(dev->CreateDepthStencilSurface(w,h,D3DFMT_D16,D3DMULTISAMPLE_NONE,0,TRUE,&depth,nullptr))||
           FAILED(dev->CreateStateBlock(D3DSBT_ALL,&state)))return fail();
        width=w;height=h;return true;
    }
    bool Draw(IDirect3DDevice9* dev,const Calls& calls,IDirect3DSurface9* back,
              const Pose (&hand)[2],const Pose (&eye)[2],const float (&blend)[2],float tanX,float tanY){
        if(!Ready()||!calls.Valid()||!back)return false;
        D3DMATRIX proj{},ident{};
        if(!Projection(tanX,tanY,.015f,20.f,proj.m))return false;
        for(unsigned i=0;i<4;++i)ident.m[i][i]=1;
        // Validate all poses before changing device state or writing any pixels.
        for(unsigned h=0;h<2;++h)for(unsigned e=0;e<2;++e)
            if(!Transform(mesh[h],blend[h],hand[h],eye[e]))return false;
        StateGuard<> guard(dev,state,calls);if(!guard.valid)return false;
        if(FAILED(dev->SetDepthStencilSurface(nullptr)))return false;
        for(unsigned i=1;i<guard.count;++i)if(FAILED(calls.target(dev,i,nullptr)))return false;
        if(FAILED(calls.target(dev,0,back))||FAILED(dev->SetDepthStencilSurface(depth)))return false;
        // Private depth: hands self-occlude correctly, but intentionally overlay
        // world depth (no unrelated world-depth format/space assumptions).
        if(FAILED(calls.clear(dev,0,nullptr,D3DCLEAR_ZBUFFER,0,1.f,0)))return false;
        if(FAILED(calls.vs(dev,nullptr))||FAILED(calls.ps(dev,nullptr))||
           FAILED(dev->SetFVF(D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1)))return false;
        // Explicit fixed-function baseline, independent of the preceding UE pass.
        struct RS{D3DRENDERSTATETYPE key;DWORD value;};
        const RS rs[]={{D3DRS_ZENABLE,D3DZB_TRUE},{D3DRS_ZWRITEENABLE,TRUE},{D3DRS_ZFUNC,D3DCMP_LESSEQUAL},
            {D3DRS_ALPHABLENDENABLE,FALSE},{D3DRS_SEPARATEALPHABLENDENABLE,FALSE},
            {D3DRS_ALPHATESTENABLE,FALSE},{D3DRS_CULLMODE,D3DCULL_NONE},{D3DRS_LIGHTING,FALSE},
            {D3DRS_FOGENABLE,FALSE},{D3DRS_STENCILENABLE,FALSE},{D3DRS_SCISSORTESTENABLE,FALSE},
            {D3DRS_CLIPPING,TRUE},{D3DRS_VERTEXBLEND,D3DVBF_DISABLE},{D3DRS_INDEXEDVERTEXBLENDENABLE,FALSE},
            {D3DRS_CLIPPLANEENABLE,0},{D3DRS_COLORWRITEENABLE,15},{D3DRS_SRGBWRITEENABLE,FALSE},
            {D3DRS_FILLMODE,D3DFILL_SOLID},{D3DRS_DEPTHBIAS,0},{D3DRS_SLOPESCALEDEPTHBIAS,0}};
        for(const auto& r:rs)if(FAILED(dev->SetRenderState(r.key,r.value)))return false;
        if(FAILED(dev->SetTransform(D3DTS_WORLD,&ident))||FAILED(dev->SetTransform(D3DTS_VIEW,&ident))||
           FAILED(dev->SetTransform(D3DTS_PROJECTION,&proj))||FAILED(calls.texture(dev,0,texture)))return false;
        if(FAILED(dev->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE))||
           FAILED(dev->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE))||
           FAILED(dev->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_DIFFUSE))||
           FAILED(dev->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1))||
           FAILED(dev->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_DIFFUSE))||
           FAILED(dev->SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,0))||
           FAILED(dev->SetTextureStageState(0,D3DTSS_RESULTARG,D3DTA_CURRENT))||
           FAILED(dev->SetTextureStageState(0,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_DISABLE))||
           FAILED(dev->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE)))return false;
        dev->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);
        dev->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);
        dev->SetSamplerState(0,D3DSAMP_MIPFILTER,D3DTEXF_NONE);
        dev->SetSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP);
        dev->SetSamplerState(0,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP);
        dev->SetSamplerState(0,D3DSAMP_SRGBTEXTURE,FALSE);
        draws=0;
        for(unsigned e=0;e<2;++e){
            D3DVIEWPORT9 vp{e*width/2,0,width/2,height,0,1};
            if(FAILED(calls.viewport(dev,&vp)))return false;
            for(unsigned h=0;h<2;++h){
                if(!Transform(mesh[h],blend[h],hand[h],eye[e]))return false;
                if(FAILED(calls.draw(dev,D3DPT_TRIANGLELIST,0,UINT(mesh[h].source.size()),UINT(mesh[h].indices.size()/3),
                    mesh[h].indices.data(),D3DFMT_INDEX32,mesh[h].scratch.data(),sizeof(Vertex))))return false;
                ++draws;
            }
        }
        return guard.Restore();
    }
};
} // namespace gloves
