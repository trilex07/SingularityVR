#pragma once
#include <windows.h>
#include <unknwn.h>
#include <cstring>
#include <cassert>
#include <cstdio>
constexpr DWORD D3DCLEAR_ZBUFFER=2;
static void Log(const char*,...){}
struct IDirect3DVertexShader9: IUnknown {HRESULT GetFunction(void*,UINT*){assert(false);return E_FAIL;}};
struct IDirect3DPixelShader9: IUnknown {HRESULT GetFunction(void*,UINT*){assert(false);return E_FAIL;}};
enum D3DRESOURCETYPE {D3DRTYPE_TEXTURE};
enum D3DFORMAT {D3DFMT_A16B16G16R16F=113,D3DFMT_R32F=114};
struct D3DSURFACE_DESC {UINT Width=0,Height=0;D3DFORMAT Format=D3DFMT_R32F;};
struct IDirect3DBaseTexture9 {unsigned refs=1;D3DRESOURCETYPE GetType(){return D3DRTYPE_TEXTURE;}void Release(){assert(refs>1);--refs;}};
struct IDirect3DTexture9:IDirect3DBaseTexture9 {D3DSURFACE_DESC desc;HRESULT GetLevelDesc(UINT level,D3DSURFACE_DESC* out){assert(level==0);*out=desc;return S_OK;}};
struct IDirect3DSurface9 {unsigned refs=1;D3DSURFACE_DESC desc;HRESULT GetDesc(D3DSURFACE_DESC* out){*out=desc;return S_OK;}void Release(){assert(refs>1);--refs;}};
struct IDirect3DDevice9 {
    float vp[16]={.5f,0,0,0,0,1,0,0,-.5f,0,1.001f,1,0,0,-10.01f,0};
    float ps[32]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,10,
                  1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    float depth[4]={9.99f,.001f,1.f/9.99f,.001f/9.99f};
    IDirect3DTexture9 color,map;IDirect3DSurface9 rt;
    RECT sc={0,0,100,100};unsigned reads=0,writes=0,resourceReads=0;
    bool failFirst=false,failRestore=false,failTexture=false;
    IDirect3DDevice9(){color.desc={200,100,D3DFMT_A16B16G16R16F};map.desc={512,512,D3DFMT_R32F};rt.desc=color.desc;}
    HRESULT GetVertexShaderConstantF(UINT r,float* out,UINT n){assert(r==0&&n==4);++reads;memcpy(out,vp,64);return S_OK;}
    HRESULT GetPixelShaderConstantF(UINT r,float* out,UINT n){++reads;if(r==2){assert(n==1);memcpy(out,depth,16);}else{assert(r==3&&n==8);memcpy(out,ps,128);}return S_OK;}
    HRESULT SetPixelShaderConstantF(UINT r,const float* in,UINT n){assert(r==3&&n==8);++writes;memcpy(ps,in,128);return (writes==1&&failFirst)||(writes==2&&failRestore)?E_FAIL:S_OK;}
    HRESULT GetTexture(UINT index,IDirect3DBaseTexture9** out){++resourceReads;assert(index<2);*out=index?&map:&color;++(*out)->refs;return failTexture?E_FAIL:S_OK;}
    HRESULT GetRenderTarget(UINT index,IDirect3DSurface9** out){assert(index==0);++resourceReads;*out=&rt;++rt.refs;return S_OK;}
    HRESULT GetScissorRect(RECT* out){++resourceReads;*out=sc;return S_OK;}
};
