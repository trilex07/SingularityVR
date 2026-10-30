#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <d3d9.h>
namespace texbridge {
using Forward=HRESULT (STDMETHODCALLTYPE*)(IDirect3DDevice9*,DWORD,IDirect3DBaseTexture9*);
using Bind=HRESULT (WINAPI*)(IDirect3DDevice9*,DWORD,IDirect3DBaseTexture9*,Forward,unsigned,unsigned);
using Lifecycle=void (WINAPI*)(IDirect3DDevice9*,unsigned); // 0 init, 1 pre-reset/release, 2 post-reset
constexpr unsigned version=1, protectedSource=1;
// Project-owned COM metadata, not an engine offset. Metadata dies with source.
inline const GUID positiveId={0x3558dca8,0x2fe4,0x4988,{0x80,0x56,0xc6,0xec,0xf3,0x89,0x98,0x67}};
inline const GUID nativeId={0xfda244ce,0x8924,0x496f,{0xa3,0x2d,0x9c,0xfa,0x9e,0x06,0xc7,0x1c}};
inline const GUID seekerId={0x54f27a71,0xc8f4,0x442a,{0xb1,0x87,0x43,0x26,0x91,0xa2,0x77,0x34}};
inline void Invalidate(IDirect3DTexture9* t){if(t){t->FreePrivateData(positiveId);t->FreePrivateData(nativeId);t->FreePrivateData(seekerId);}}
}
