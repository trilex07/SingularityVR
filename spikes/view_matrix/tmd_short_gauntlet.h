#pragma once
// Original TMD material-1 geometry, with a short source sleeve and proximal
// cap. Index-only: native VB, skin palette, UVs, shaders and moving TMD bones
// remain bound. Private SVTG is never distributed in Git.
#include "glove_native.h"

namespace tmdgauntlet {
struct Data { std::vector<std::uint16_t> indices; unsigned capTriangles=0; };
inline bool Decode(const unsigned char* p, std::size_t bytes, Data& out) {
    if (!p || bytes < 24 || std::memcmp(p,"SVTG",4) || gloves::U32(p+4)!=1 ||
        gloves::U32(p+8)!=7444 || gloves::U32(p+12)!=7110 ||
        gloves::U32(p+16)!=23 || gloves::U32(p+20)!=36369 ||
        bytes != 24 + std::size_t(7110)*6) return false;
    Data next; next.indices.resize(7110*3); next.capTriangles=23;
    std::memcpy(next.indices.data(),p+24,next.indices.size()*2);
    for (unsigned i=0;i<next.indices.size();i+=3) {
        const auto a=next.indices[i],b=next.indices[i+1],c=next.indices[i+2];
        if (a>=7444 || b>=7444 || c>=7444 || a==b || b==c || a==c) return false;
    }
    out=std::move(next);return true;
}
inline bool Eligible(bool feature, bool ready, bool cuffReady, bool independent,
                     int isolate, bool scene, bool shadow, bool cinematic) {
    return feature && ready && cuffReady && independent && isolate==2 &&
        scene && !shadow && !cinematic;
}
inline bool IsRightForearmTail(unsigned verts,unsigned first,unsigned triangles) {
    // Original TMD material 2: 17 faces, exclusively r_arm_elbow/forearm.
    return (verts==7444 || verts==37) && first==36318 && triangles==17;
}
class Renderer {
    Data data; IDirect3DIndexBuffer9* ib=nullptr; bool ready=false,failed=false;
public:
    bool Load(const char* path) {
        ReleaseGPU();ready=false;std::vector<unsigned char> bytes;Data next;
        if (!gloves::Read(path,bytes,64*1024) || !Decode(bytes.data(),bytes.size(),next)) return false;
        data=std::move(next);ready=true;return true;
    }
    bool Ready() const {return ready;}
    unsigned Triangles() const {return ready ? (unsigned)data.indices.size()/3 : 0;}
    void ReleaseGPU(){gloves::Release(ib);failed=false;}
    IDirect3DIndexBuffer9* Ensure(IDirect3DDevice9* dev) {
        if (!ready || failed || !dev) return nullptr;
        if (ib) return ib;
        const UINT bytes=(UINT)data.indices.size()*2;void* dst=nullptr;
        HRESULT hr=dev->CreateIndexBuffer(bytes,D3DUSAGE_WRITEONLY,D3DFMT_INDEX16,D3DPOOL_DEFAULT,&ib,nullptr);
        if (SUCCEEDED(hr) && ib) hr=ib->Lock(0,bytes,&dst,0);else hr=E_FAIL;
        if (SUCCEEDED(hr)) {if(dst)std::memcpy(dst,data.indices.data(),bytes);else hr=E_POINTER;
            const HRESULT unlock=ib->Unlock();if(FAILED(unlock))hr=unlock;}
        if (FAILED(hr)){gloves::Release(ib);failed=true;return nullptr;}
        return ib;
    }
};
} // namespace tmdgauntlet
