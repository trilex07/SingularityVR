#pragma once
#include <d3d9.h>
#include <cstdint>
#include "fp_shadow.h"

// A depth-only, near-plane sibling must not leave the *native* pose in the
// scene when the late first-person color uses a different independent pose.
// This removes only that redundant prime, never a color or shadow-map draw.
// Geometry is learned AFTER successful late stereo submission/restoration.
namespace trackedprime {
struct Owner {
    uintptr_t pawn=0, weapon=0;
    uint32_t frame=0;
    bool active=false;
};
struct Draw {
    fpshadow::DrawKey key{};
    bool triangles=false, foreground=false;
};
struct Receipt {
    bool sibling=false;
    DWORD z=0,write=0,color=~0u,stencil=1;
};
class Cache {
    struct Entry {fpshadow::DrawKey key{};uint32_t frame=0;};
    Entry entries[32]{};
    unsigned count=0;
    uintptr_t pawn=0,weapon=0;
    IDirect3DDevice9* viewportDevice=nullptr; // borrowed, cleared before Reset
    D3DVIEWPORT9 primeViewport{};
    bool viewportValid=false;
    static void Release(fpshadow::DrawKey& k) {
        if(k.vb)static_cast<IDirect3DVertexBuffer9*>(const_cast<void*>(k.vb))->Release();
        if(k.ib)static_cast<IDirect3DIndexBuffer9*>(const_cast<void*>(k.ib))->Release();
        k={};
    }
    static bool Read(IDirect3DDevice9* d,fpshadow::DrawKey& k) {
        IDirect3DVertexBuffer9* vb=nullptr;IDirect3DIndexBuffer9* ib=nullptr;
        UINT offset=0,stride=0;
        const HRESULT v=d->GetStreamSource(0,&vb,&offset,&stride),i=d->GetIndices(&ib);
        if(FAILED(v)||FAILED(i)||!vb||!ib||!stride) {
            if(vb)vb->Release();if(ib)ib->Release();return false;
        }
        k.vb=vb;k.ib=ib;k.vbOffset=offset;k.stride=stride;return true;
    }
    void OwnerNow(const Owner& o) {
        if(o.pawn!=pawn||o.weapon!=weapon){Reset();pawn=o.pawn;weapon=o.weapon;}
    }
public:
    Cache()=default;
    Cache(const Cache&)=delete;Cache& operator=(const Cache&)=delete;
    void Reset() {for(unsigned i=0;i<count;++i)Release(entries[i].key);count=0;pawn=weapon=0;
        viewportDevice=nullptr;viewportValid=false;}
    void ObserveViewport(IDirect3DDevice9* d,const D3DVIEWPORT9* requested,HRESULT result) {
        viewportValid=false;viewportDevice=nullptr;
        // Some D3D9 drivers canonicalize requested 0/0 to actual 0/.001.
        // Measure that state after a successful native request; never guess an
        // epsilon that could classify a different depth range as a prime.
        if(!requested||FAILED(result)||requested->MinZ!=0.f||requested->MaxZ!=0.f)return;
        if(FAILED(d->GetViewport(&primeViewport)))return;
        viewportValid=primeViewport.X==requested->X&&primeViewport.Y==requested->Y&&
            primeViewport.Width==requested->Width&&primeViewport.Height==requested->Height;
        viewportDevice=d;
    }
    // No mutations to D3D state, shader constants, stencil, COM bindings or poses.
    bool Skip(IDirect3DDevice9* d,const Owner& o,Draw draw,Receipt* receipt=nullptr) {
        if(receipt)*receipt={};
        OwnerNow(o);
        if(!o.active||!o.pawn||!o.weapon||!count||!draw.triangles||draw.foreground||
           draw.key.layoutShadow||!draw.key.rtIsScene)return false;
        D3DVIEWPORT9 vp{};
        if(!viewportValid||viewportDevice!=d||FAILED(d->GetViewport(&vp))||
           vp.X!=primeViewport.X||vp.Y!=primeViewport.Y||vp.Width!=primeViewport.Width||
           vp.Height!=primeViewport.Height||vp.MinZ!=primeViewport.MinZ||vp.MaxZ!=primeViewport.MaxZ)return false;
        if(!Read(d,draw.key))return false;
        bool found=false;
        for(unsigned i=0;i<count;++i)
            if(uint32_t(o.frame-entries[i].frame)<=2&&fpshadow::SameGeometry(draw.key,entries[i].key))
                {found=true;break;}
        Release(draw.key);
        if(!found)return false;
        if(receipt)receipt->sibling=true;
        DWORD z=0,write=0,color=~0u,stencil=1;
        if(FAILED(d->GetRenderState(D3DRS_ZENABLE,&z))||
           FAILED(d->GetRenderState(D3DRS_ZWRITEENABLE,&write))||
           FAILED(d->GetRenderState(D3DRS_COLORWRITEENABLE,&color))||
           FAILED(d->GetRenderState(D3DRS_STENCILENABLE,&stencil))||
           z!=D3DZB_TRUE||!write||color||stencil){
            if(receipt){receipt->z=z;receipt->write=write;receipt->color=color;receipt->stencil=stencil;}
            return false;
        }
        return true;
    }
    bool Learn(IDirect3DDevice9* d,const Owner& o,Draw draw,bool restoredStereoColor) {
        OwnerNow(o);
        if(!o.active||!o.pawn||!o.weapon||!restoredStereoColor||!draw.triangles||
           !draw.foreground||!draw.key.rtIsScene||draw.key.layoutShadow)return false;
        D3DVIEWPORT9 vp{};DWORD color=0;
        if(FAILED(d->GetViewport(&vp))||vp.MaxZ<=vp.MinZ||
           FAILED(d->GetRenderState(D3DRS_COLORWRITEENABLE,&color))||!(color&7))return false;
        if(!Read(d,draw.key))return false;
        for(unsigned i=0;i<count;++i)if(fpshadow::SameGeometry(draw.key,entries[i].key)) {
            entries[i].frame=o.frame;Release(draw.key);return false;
        }
        // Reclaim stale entries without retaining a dead/reused COM identity.
        for(unsigned i=0;i<count;++i)if(uint32_t(o.frame-entries[i].frame)>2) {
            Release(entries[i].key);entries[i]={draw.key,o.frame};return true;
        }
        if(count<32){entries[count++]={draw.key,o.frame};return true;}
        Release(draw.key);return false;
    }
};
} // namespace trackedprime
