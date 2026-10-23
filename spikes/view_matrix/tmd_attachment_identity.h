#pragma once
#include <d3d9.h>
#include <cstdint>
namespace tmdattachment {
inline uint32_t Crc(const unsigned char* p,unsigned n){
    uint32_t c=~0u;for(unsigned i=0;i<n;++i){c^=p[i];for(unsigned b=0;b<8;++b)c=(c>>1)^(0xedb88320u&-(c&1u));}return ~c;
}
struct Signature {unsigned vertices,faces;uint32_t crc;};
constexpr Signature signatures[]={
 {729,1096,0x5ffe23c5},{1368,1958,0x60abd596},{843,1274,0x76144c0c},
 {389,588,0xdcfcd99e},{207,270,0x5e1956ae},{251,344,0x5cb3984c},
 {432,572,0x3c10e546},{107,158,0x11bd4ce8},{107,158,0x604569dd},
 {118,108,0x6e9767c9},{190,290,0x0e14077c}};
inline bool Candidate(unsigned v,unsigned f){for(auto s:signatures)if(s.vertices==v&&s.faces==f)return true;return false;}
inline bool Verified(unsigned v,unsigned f,uint32_t crc){for(auto s:signatures)if(s.vertices==v&&s.faces==f&&s.crc==crc)return true;return false;}
// Original socket attachments, each index stream uniquely round-tripped in cooked package.
struct Identity {
    struct Entry{IDirect3DIndexBuffer9* buffer=nullptr;unsigned v=0,f=0;bool matches=false;};
    Entry entries[16]{};
    ~Identity(){Reset();}
    void Reset(){for(auto& e:entries){if(e.buffer)e.buffer->Release();e=Entry{};}}
    bool Match(IDirect3DDevice9* dev,unsigned v,unsigned f){
        if(!Candidate(v,f))return false;
        IDirect3DIndexBuffer9* current=nullptr;
        if(FAILED(dev->GetIndices(&current))||!current)return false;
        Entry* free=nullptr;
        for(auto& e:entries){if(e.buffer==current){current->Release();return e.matches&&e.v==v&&e.f==f;}if(!e.buffer&&!free)free=&e;}
        if(!free){current->Release();return false;}
        free->buffer=current;free->v=v;free->f=f;
        D3DINDEXBUFFER_DESC desc{};void* bytes=nullptr;const unsigned length=f*6;
        if(FAILED(current->GetDesc(&desc))||desc.Format!=D3DFMT_INDEX16||desc.Size!=length)return false;
        if(FAILED(current->Lock(0,length,&bytes,D3DLOCK_READONLY)))return false;
        const bool equal=bytes&&Verified(v,f,Crc(static_cast<unsigned char*>(bytes),length));
        free->matches=SUCCEEDED(current->Unlock())&&equal;return free->matches;
    }
};
}
