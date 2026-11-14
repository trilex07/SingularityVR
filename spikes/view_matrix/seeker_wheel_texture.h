#pragma once

#include <d3d9.h>
#include "seeker_wheel_pixels.generated.h"

namespace seekericon {
inline bool UploadTexture(IDirect3DDevice9* dev, IDirect3DTexture9** out) {
    IDirect3DTexture9* tex=nullptr;
    if(FAILED(dev->CreateTexture(TextureSide,TextureSide,1,D3DUSAGE_DYNAMIC,D3DFMT_A8R8G8B8,
                                 D3DPOOL_DEFAULT,&tex,nullptr))||!tex)return false;
    D3DLOCKED_RECT lock{};
    if(FAILED(tex->LockRect(0,&lock,nullptr,D3DLOCK_DISCARD))){tex->Release();return false;}
    if(lock.Pitch < static_cast<INT>(TextureSide*sizeof(DWORD))){
        tex->UnlockRect(0);tex->Release();return false;
    }
    for(unsigned y=0;y<TextureSide;++y){
        // Matte RGB under zero alpha prevents dark fringing under linear filtering.
        auto* row=reinterpret_cast<DWORD*>(static_cast<BYTE*>(lock.pBits)+y*lock.Pitch);
        for(unsigned x=0;x<TextureSide;++x) row[x]=0x00B4AFA3;
    }
    auto* span=TextureSpans;
    while(span[1]){
        const unsigned offset=*span++;
        const unsigned count=*span++;
        auto* row=reinterpret_cast<DWORD*>(static_cast<BYTE*>(lock.pBits)+(offset/TextureSide)*lock.Pitch);
        auto* dst=row+offset%TextureSide;
        for(unsigned n=0;n<count;++n){
            const DWORD value=*span++, tone=value&255;
            *dst++=((value&0xff00)<<16)|((tone+7)<<16)|((tone+2)<<8)|(tone-10);
        }
    }
    if(FAILED(tex->UnlockRect(0))){tex->Release();return false;}
    *out=tex;
    return true;
}
}
