#pragma once
#include <vector>
#include <cstring>
#include <cmath>
#include <cstdint>

namespace volkhd {
constexpr unsigned SourceVertices=8297,SourceTriangles=10835,Vertices=12085,Triangles=12727;
inline unsigned Hash(const unsigned char* p,size_t n){unsigned h=2166136261u;while(n--)h=(h^*p++)*16777619u;return h;}
struct VolkProfile {
    static constexpr unsigned SourceVertices=8297,SourceTriangles=10835,Vertices=12085,Triangles=12727,Slots=5,SourceVbHash=4146942701u,SourceIbHash=754008012u;
    static constexpr bool NativePacked=false;
    static const char* Magic(){return "SVVH";}
};
struct ValkyrieProfile {
    static constexpr unsigned SourceVertices=11387,SourceTriangles=12834,Vertices=13965,Triangles=14116,Slots=6,SourceVbHash=3607008755u,SourceIbHash=615094726u;
    static constexpr bool NativePacked=true;
    static const char* Magic(){return "SVVK";}
};
struct KasimovProfile {
    static constexpr unsigned SourceVertices=11389,SourceTriangles=12942,Vertices=12637,Triangles=13722,Slots=6,SourceVbHash=2571367080u,SourceIbHash=855862860u;
    static constexpr bool NativePacked=true;
    static const char* Magic(){return "SVKS";}
};
struct CenturionProfile {
    static constexpr unsigned SourceVertices=5799,SourceTriangles=6767,Vertices=8051,Triangles=7927,Slots=5,SourceVbHash=483118394u,SourceIbHash=147026316u;
    static constexpr bool NativePacked=true;
    static const char* Magic(){return "SVCT";}
};
template<class Profile> struct NativeAsset {
    static constexpr unsigned SourceVertices=Profile::SourceVertices,SourceTriangles=Profile::SourceTriangles,Vertices=Profile::Vertices,Triangles=Profile::Triangles;
    std::vector<unsigned char> bytes;
    bool Load(const unsigned char* p,size_t n){
        const size_t size=28+Vertices*32+Triangles*6;
        if(!p||n!=size||memcmp(p,Profile::Magic(),4))return false;
        unsigned h[6];memcpy(h,p+4,24);
        if(h[0]!=1||h[1]!=Vertices||h[2]!=Triangles*3||h[3]!=SourceVertices||h[4]!=Profile::SourceIbHash||Hash(p+28,n-28)!=h[5])return false;
        if(Hash(p+28,SourceVertices*32)!=Profile::SourceVbHash||Hash(p+28+Vertices*32,SourceTriangles*6)!=Profile::SourceIbHash)return false;
        for(unsigned i=0;i<Vertices;++i){
            const auto* v=p+28+i*32;float xyz[3];memcpy(xyz,v,12);
            for(float x:xyz)if(!std::isfinite(x)||fabsf(x)>1000)return false;
            unsigned sum=0;for(unsigned j=0;j<4;++j){if(v[20+j]>=Profile::Slots)return false;sum+=v[24+j];}
            if(sum!=255)return false;
        }
        for(unsigned i=0;i<Triangles*3;++i){uint16_t index;memcpy(&index,p+28+Vertices*32+i*2,2);if(index>=Vertices)return false;}
        // Transactional: failed replacement must not destroy the accepted asset.
        std::vector<unsigned char> staged(p+28,p+n);bytes.swap(staged);return true;
    }
};

using Asset=NativeAsset<VolkProfile>;

#ifdef _D3D9_H_
// Keeps original declaration, shaders, constants, texture stages, RT/depth,
// blend/cull and viewport. Only stream0/IB change, scoped to a source draw.
template<class Profile> class NativeRenderer {
    static constexpr unsigned SourceVertices=Profile::SourceVertices,SourceTriangles=Profile::SourceTriangles,Vertices=Profile::Vertices,Triangles=Profile::Triangles;
    IDirect3DVertexBuffer9* vb=nullptr;
    IDirect3DIndexBuffer9* ib=nullptr;
    bool failed=false;
public:
    NativeAsset<Profile> asset;
    const char* receipt="not-drawn";
    struct SourceAudit {
        UINT offset=0,stride=0,freq=0,count=0,mask=0;
        bool expectedIb=false;
        HRESULT streamHr=E_FAIL,indexHr=E_FAIL,freqHr=E_FAIL,declHr=E_FAIL;
        D3DVERTEXELEMENT9 elements[MAXD3DDECLLENGTH]{};
    } audit;
    void ReleaseGPU(){if(vb)vb->Release();if(ib)ib->Release();vb=nullptr;ib=nullptr;failed=false;}
    ~NativeRenderer(){ReleaseGPU();}
    bool Ensure(IDirect3DDevice9* dev){
        if(failed||asset.bytes.empty())return false;
        if(vb&&ib)return true;
        failed=true;
        if(FAILED(dev->CreateVertexBuffer(Vertices*32,D3DUSAGE_WRITEONLY,0,D3DPOOL_DEFAULT,&vb,nullptr))||
           FAILED(dev->CreateIndexBuffer(Triangles*6,D3DUSAGE_WRITEONLY,D3DFMT_INDEX16,D3DPOOL_DEFAULT,&ib,nullptr)))return false;
        void* p=nullptr;
        if(FAILED(vb->Lock(0,0,&p,0)))return false;
        memcpy(p,asset.bytes.data(),Vertices*32);
        if(FAILED(vb->Unlock())||FAILED(ib->Lock(0,0,&p,0)))return false;
        memcpy(p,asset.bytes.data()+Vertices*32,Triangles*6);
        if(FAILED(ib->Unlock()))return false;
        failed=false;return true;
    }
    template<class Draw> HRESULT DrawNative(IDirect3DDevice9* dev,Draw draw,bool& replaced,IDirect3DIndexBuffer9* expectedIndices=nullptr){
        replaced=false;
        IDirect3DVertexBuffer9* source=nullptr;IDirect3DIndexBuffer9* indices=nullptr;
        IDirect3DVertexDeclaration9* decl=nullptr;UINT offset=0,stride=0,freq=0,count=MAXD3DDECLLENGTH;
        D3DVERTEXELEMENT9 elements[MAXD3DDECLLENGTH]{};
        audit={};
        audit.streamHr=dev->GetStreamSource(0,&source,&offset,&stride);
        audit.indexHr=dev->GetIndices(&indices);
        audit.freqHr=dev->GetStreamSourceFreq(0,&freq);
        audit.declHr=dev->GetVertexDeclaration(&decl);
        if(SUCCEEDED(audit.declHr)&&decl)audit.declHr=decl->GetDeclaration(elements,&count);
        audit.offset=offset;audit.stride=stride;audit.freq=freq;
        audit.expectedIb=indices&&(!expectedIndices||indices==expectedIndices);
        audit.count=SUCCEEDED(audit.declHr)?std::min(count,UINT(MAXD3DDECLLENGTH)):0;
        memcpy(audit.elements,elements,audit.count*sizeof(elements[0]));
        bool compatible=SUCCEEDED(audit.streamHr)&&source&&stride==32&&offset==0&&
            SUCCEEDED(audit.indexHr)&&audit.expectedIb&&SUCCEEDED(audit.freqHr)&&freq==1&&
            SUCCEEDED(audit.declHr)&&decl;
        unsigned mask=0,uvMask=0;
        const BYTE packed=Profile::NativePacked?D3DDECLTYPE_UBYTE4:D3DDECLTYPE_UBYTE4N;
        for(unsigned i=0;compatible&&i<count&&elements[i].Stream!=0xff;++i){
            const auto& e=elements[i];if(e.Stream){compatible=false;break;}
            if(Profile::NativePacked){
                const BYTE usage=e.Offset==0?D3DDECLUSAGE_POSITION:e.Offset==12?D3DDECLUSAGE_TANGENT:
                    e.Offset==16?D3DDECLUSAGE_NORMAL:e.Offset==20?D3DDECLUSAGE_BLENDINDICES:
                    e.Offset==24?D3DDECLUSAGE_BLENDWEIGHT:D3DDECLUSAGE_TEXCOORD;
                if(e.Method!=D3DDECLMETHOD_DEFAULT||e.Usage!=usage||
                   (e.Offset==28?e.UsageIndex>3:e.UsageIndex!=0)){compatible=false;break;}
                if(e.Offset==28)uvMask|=1u<<e.UsageIndex;
            }
            if(e.Offset==0&&e.Type==D3DDECLTYPE_FLOAT3)mask|=1;
            else if(e.Offset==12&&e.Type==packed)mask|=2;
            else if(e.Offset==16&&e.Type==packed)mask|=4;
            else if(e.Offset==20&&e.Type==D3DDECLTYPE_UBYTE4)mask|=8;
            else if(e.Offset==24&&e.Type==D3DDECLTYPE_UBYTE4N)mask|=16;
            else if(e.Offset==28&&e.Type==D3DDECLTYPE_FLOAT16_2)mask|=32;
            else compatible=false;
        }
        if(Profile::NativePacked&&uvMask!=15)compatible=false;
        audit.mask=mask;
        if(decl)decl->Release();
        HRESULT hr=D3D_OK;
        receipt=(!compatible||mask!=63)?"source-layout-or-IB":asset.bytes.empty()?"missing-cache":"GPU-unavailable";
        if(compatible&&mask==63&&Ensure(dev)){
            const HRESULT a=dev->SetStreamSource(0,vb,0,32);
            const HRESULT b=SUCCEEDED(a)?dev->SetIndices(ib):a;
            receipt="bind-failed";
            if(SUCCEEDED(a)&&SUCCEEDED(b)){replaced=true;hr=draw(Vertices,Triangles);receipt=SUCCEEDED(hr)?"HD-submitted":"HD-draw-failed";}
            // Always restore BOTH, including a partial binding failure.
            const HRESULT r0=dev->SetStreamSource(0,source,offset,stride);
            const HRESULT r1=dev->SetIndices(indices);
            if(FAILED(r0)||FAILED(r1)){failed=true;hr=E_FAIL;replaced=true;receipt="restore-failed";}
        }
        if(source)source->Release();if(indices)indices->Release();
        return replaced?hr:draw(SourceVertices,SourceTriangles);
    }
};
using Renderer=NativeRenderer<VolkProfile>;
#endif
}

namespace valkyriehd {
using Profile=volkhd::ValkyrieProfile;
constexpr unsigned SourceVertices=Profile::SourceVertices,SourceTriangles=Profile::SourceTriangles,Vertices=Profile::Vertices,Triangles=Profile::Triangles;
using Asset=volkhd::NativeAsset<Profile>;
#ifdef _D3D9_H_
using Renderer=volkhd::NativeRenderer<Profile>;
#endif
}

namespace kasimovhd {
using Profile=volkhd::KasimovProfile;
constexpr unsigned SourceVertices=Profile::SourceVertices,SourceTriangles=Profile::SourceTriangles,Vertices=Profile::Vertices,Triangles=Profile::Triangles;
using Asset=volkhd::NativeAsset<Profile>;
#ifdef _D3D9_H_
using Renderer=volkhd::NativeRenderer<Profile>;
#endif
}

namespace centurionhd {
using Profile=volkhd::CenturionProfile;
constexpr unsigned SourceVertices=Profile::SourceVertices,SourceTriangles=Profile::SourceTriangles,Vertices=Profile::Vertices,Triangles=Profile::Triangles;
using Asset=volkhd::NativeAsset<Profile>;
#ifdef _D3D9_H_
using Renderer=volkhd::NativeRenderer<Profile>;
// Independent idle-body witness: the existing reload witness starts only once
// a cylinder animation is active. Do not require a reload before HD can render.
struct IndexWitness {
    IDirect3DIndexBuffer9* buffer=nullptr;
    bool valid=false;unsigned attempts=0;
    ~IndexWitness(){Reset();}
    void Reset(){if(buffer)buffer->Release();buffer=nullptr;valid=false;attempts=0;}
    bool Observe(IDirect3DDevice9* dev){
        IDirect3DIndexBuffer9* current=nullptr;
        if(FAILED(dev->GetIndices(&current))||!current)return false;
        if(current!=buffer){
            if(buffer)buffer->Release();buffer=current;current=nullptr;valid=false;
            if(attempts++<4){
                D3DINDEXBUFFER_DESC desc{};void* data=nullptr;
                constexpr UINT bytes=SourceTriangles*6;
                if(SUCCEEDED(buffer->GetDesc(&desc))&&desc.Format==D3DFMT_INDEX16&&desc.Size==bytes&&
                   SUCCEEDED(buffer->Lock(0,bytes,&data,D3DLOCK_READONLY))){
                    const bool match=data&&volkhd::Hash((const unsigned char*)data,bytes)==Profile::SourceIbHash;
                    valid=SUCCEEDED(buffer->Unlock())&&match;
                }
            }
        }
        if(current)current->Release();return valid;
    }
};
#endif
}
