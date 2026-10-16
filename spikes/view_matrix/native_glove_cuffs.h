#pragma once
#include "glove_native.h"
namespace glovecuffs {
inline int Profile(unsigned verts,unsigned first,unsigned triangles){
    if(first!=0)return -1;
    if((verts==3314||verts==2924)&&triangles==4830)return 0;
    if((verts==7444||verts==2626)&&triangles==4402)return 1;
    return -1;
}
inline bool Eligible(bool enabled,bool native,bool ready,bool independent,int isolate,
    bool gameplay,bool focused,bool scene,bool shadow,bool paused,bool menu,bool cinematic){
    return enabled&&native&&ready&&independent&&isolate==2&&gameplay&&focused&&scene&&!shadow&&!paused&&!menu&&!cinematic;
}
struct Mesh {std::vector<std::uint16_t> indices;unsigned caps=0,sourceIndices=0;};
struct Data {Mesh mesh[2];};
inline bool Decode(const unsigned char* p,std::size_t size,Data& out){
    if(!p||size<16||std::memcmp(p,"SVNC",4)||gloves::U32(p+4)!=1||gloves::U32(p+8)!=2||gloves::U32(p+12))return false;
    Data next;std::size_t at=16;
    constexpr unsigned vertices[]={2924,2626},sourceTris[]={4830,4402},sourceIndices[]={16512,36369};
    for(unsigned i=0;i<2;++i){
        if(at+24>size||gloves::U32(p+at)!=i+1||gloves::U32(p+at+4)!=vertices[i]||
            gloves::U32(p+at+8)!=sourceTris[i]||gloves::U32(p+at+12)!=sourceIndices[i])return false;
        const unsigned count=gloves::U32(p+at+16),caps=gloves::U32(p+at+20);at+=24;
        if(count<6||count%3||count>18000||caps<2||caps>512||caps*3>=count||at+std::size_t(count)*2>size)return false;
        auto& m=next.mesh[i];m.indices.resize(count);m.caps=caps;m.sourceIndices=sourceIndices[i];
        std::memcpy(m.indices.data(),p+at,count*2);at+=count*2;
        for(unsigned j=0;j<count;j+=3){
            const auto a=m.indices[j],b=m.indices[j+1],c=m.indices[j+2];
            if(a>=vertices[i]||b>=vertices[i]||c>=vertices[i]||a==b||b==c||a==c)return false;
        }
    }
    if(at!=size)return false;
    out=std::move(next);return true;
}
class Renderer {
    Data data;IDirect3DIndexBuffer9* ib[2]{};bool cpu=false,failed[2]{};
public:
    bool Load(const char* path){
        ReleaseGPU();cpu=false;std::vector<unsigned char> bytes;Data next;
        if(!gloves::Read(path,bytes,128*1024)||!Decode(bytes.data(),bytes.size(),next))return false;
        data=std::move(next);cpu=true;return true;
    }
    bool Ready()const{return cpu;}
    unsigned Triangles(int profile)const{return profile>=0&&profile<2?(unsigned)data.mesh[profile].indices.size()/3:0;}
    unsigned SourceBytes(int profile)const{return profile>=0&&profile<2?data.mesh[profile].sourceIndices*2:0;}
    void ReleaseGPU(){for(unsigned i=0;i<2;++i){gloves::Release(ib[i]);failed[i]=false;}}
    IDirect3DIndexBuffer9* Ensure(IDirect3DDevice9* dev,int profile){
        if(!cpu||profile<0||profile>1||failed[profile])return nullptr;
        if(ib[profile])return ib[profile];
        const auto& indices=data.mesh[profile].indices;const UINT bytes=(UINT)indices.size()*2;void* target=nullptr;
        HRESULT hr=dev->CreateIndexBuffer(bytes,D3DUSAGE_WRITEONLY,D3DFMT_INDEX16,D3DPOOL_DEFAULT,&ib[profile],nullptr);
        if(SUCCEEDED(hr)&&ib[profile])hr=ib[profile]->Lock(0,bytes,&target,0);else hr=E_FAIL;
        if(SUCCEEDED(hr)){if(target)std::memcpy(target,indices.data(),bytes);else hr=E_POINTER;
            const HRESULT unlocked=ib[profile]->Unlock();if(FAILED(unlocked))hr=unlocked;}
        if(FAILED(hr)){gloves::Release(ib[profile]);failed[profile]=true;return nullptr;}
        return ib[profile];
    }
};
} // namespace glovecuffs
