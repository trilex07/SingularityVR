#pragma once
#include <vector>
#include <cstring>
#include <cmath>
#include <cstdint>

namespace volkhd {
constexpr unsigned SourceVertices=8297,SourceTriangles=10835,Vertices=12085,Triangles=12727;
inline unsigned Hash(const unsigned char* p,size_t n){unsigned h=2166136261u;while(n--)h=(h^*p++)*16777619u;return h;}
struct Asset {
    std::vector<unsigned char> bytes;
    bool Load(const unsigned char* p,size_t n){
        const size_t size=28+Vertices*32+Triangles*6;
        if(!p||n!=size||memcmp(p,"SVVH",4))return false;
        unsigned h[6];memcpy(h,p+4,24);
        if(h[0]!=1||h[1]!=Vertices||h[2]!=Triangles*3||h[3]!=SourceVertices||h[4]!=754008012u||Hash(p+28,n-28)!=h[5])return false;
        if(Hash(p+28,SourceVertices*32)!=4146942701u||Hash(p+28+Vertices*32,SourceTriangles*6)!=754008012u)return false;
        for(unsigned i=0;i<Vertices;++i){
            const auto* v=p+28+i*32;float xyz[3];memcpy(xyz,v,12);
            for(float x:xyz)if(!std::isfinite(x)||fabsf(x)>1000)return false;
            unsigned sum=0;for(unsigned j=0;j<4;++j){if(v[20+j]>=5)return false;sum+=v[24+j];}
            if(sum!=255)return false;
        }
        for(unsigned i=0;i<Triangles*3;++i){uint16_t index;memcpy(&index,p+28+Vertices*32+i*2,2);if(index>=Vertices)return false;}
        // Transactional: failed replacement must not destroy the accepted asset.
        std::vector<unsigned char> staged(p+28,p+n);bytes.swap(staged);return true;
    }
};

#ifdef _D3D9_H_
// Keeps original declaration, shaders, constants, texture stages, RT/depth,
// blend/cull and viewport. Only stream0/IB change, scoped to a source draw.
class Renderer {
    IDirect3DVertexBuffer9* vb=nullptr;
    IDirect3DIndexBuffer9* ib=nullptr;
    bool failed=false;
public:
    Asset asset;
    const char* receipt="not-drawn";
    void ReleaseGPU(){if(vb)vb->Release();if(ib)ib->Release();vb=nullptr;ib=nullptr;failed=false;}
    ~Renderer(){ReleaseGPU();}
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
        bool compatible=SUCCEEDED(dev->GetStreamSource(0,&source,&offset,&stride))&&source&&stride==32&&offset==0&&
            SUCCEEDED(dev->GetIndices(&indices))&&indices&&(!expectedIndices||indices==expectedIndices)&&SUCCEEDED(dev->GetStreamSourceFreq(0,&freq))&&freq==1&&
            SUCCEEDED(dev->GetVertexDeclaration(&decl))&&decl&&SUCCEEDED(decl->GetDeclaration(elements,&count));
        unsigned mask=0;
        for(unsigned i=0;compatible&&i<count&&elements[i].Stream!=0xff;++i){
            const auto& e=elements[i];if(e.Stream){compatible=false;break;}
            if(e.Offset==0&&e.Type==D3DDECLTYPE_FLOAT3)mask|=1;
            else if(e.Offset==12&&e.Type==D3DDECLTYPE_UBYTE4N)mask|=2;
            else if(e.Offset==16&&e.Type==D3DDECLTYPE_UBYTE4N)mask|=4;
            else if(e.Offset==20&&e.Type==D3DDECLTYPE_UBYTE4)mask|=8;
            else if(e.Offset==24&&e.Type==D3DDECLTYPE_UBYTE4N)mask|=16;
            else if(e.Offset==28&&e.Type==D3DDECLTYPE_FLOAT16_2)mask|=32;
            else compatible=false;
        }
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
#endif
}
