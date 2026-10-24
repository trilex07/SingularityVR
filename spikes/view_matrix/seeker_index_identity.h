#pragma once
#include "seeker_geometry.h"
#include <d3d9.h>
namespace seekergeometry {
// Source IB receipt, not a replacement resource. Bounded to eight immutable
// original buffers per device generation; never locks once-per-frame.
struct IndexIdentity {
    struct Entry{IDirect3DIndexBuffer9* ib=nullptr;bool valid=false;};
    Entry entries[8]{};
    ~IndexIdentity(){Reset();}
    void Reset(){for(auto& e:entries){if(e.ib)e.ib->Release();e={};}}
    bool Match(IDirect3DDevice9* dev){
        IDirect3DIndexBuffer9* ib=nullptr;
        if(FAILED(dev->GetIndices(&ib))||!ib){if(ib)ib->Release();return false;}
        Entry* free=nullptr;
        for(auto& e:entries){if(e.ib==ib){ib->Release();return e.valid;}if(!e.ib&&!free)free=&e;}
        if(!free){ib->Release();return false;}
        free->ib=ib;D3DINDEXBUFFER_DESC desc{};void* data=nullptr;
        if(FAILED(ib->GetDesc(&desc))||desc.Format!=D3DFMT_INDEX16||desc.Size!=IndexBytes||
           (desc.Usage&D3DUSAGE_DYNAMIC)||FAILED(ib->Lock(0,IndexBytes,&data,D3DLOCK_READONLY)))return false;
        uint32_t h=2166136261u;
        if(data)for(unsigned i=0;i<IndexBytes;++i)h=(h^((unsigned char*)data)[i])*16777619u;
        const bool match=data&&h==IndexHash;
        free->valid=SUCCEEDED(ib->Unlock())&&match;return free->valid;
    }
};
}
