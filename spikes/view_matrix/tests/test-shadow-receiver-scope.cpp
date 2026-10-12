#include <windows.h>
#include <unknwn.h>
#include <cstring>
#include <cassert>
#include <cstdio>
void Log(const char*,...){}
struct IDirect3DVertexShader9: IUnknown { HRESULT GetFunction(void*,UINT*){assert(false);return E_FAIL;} };
struct IDirect3DPixelShader9: IUnknown { HRESULT GetFunction(void*,UINT*){assert(false);return E_FAIL;} };
struct IDirect3DDevice9 {
    float vp[16]={1,0,0,0,0,1,0,0,0,0,-1,-1,0,0,-.1f,0};
    float ps[32]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,
                  1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    unsigned reads=0,writes=0;bool failFirst=false,failRestore=false;
    HRESULT GetVertexShaderConstantF(UINT r,float* p,UINT n){assert(r==0&&n==4);++reads;memcpy(p,vp,64);return S_OK;}
    HRESULT GetPixelShaderConstantF(UINT r,float* p,UINT n){assert(r==3&&n==8);++reads;memcpy(p,ps,128);return S_OK;}
    HRESULT SetPixelShaderConstantF(UINT r,const float* p,UINT n){assert(r==3&&n==8);++writes;memcpy(ps,p,128);return (writes==1&&failFirst)||(writes==2&&failRestore)?E_FAIL:S_OK;}
};
#include "../shadow_receiver_fix.h"
int main(){
    using namespace shadowfix;IDirect3DDevice9 d;float original[32];memcpy(original,d.ps,128);
    {Scope s(&d,true,8,12);}assert(!d.reads&&!d.writes);
    enabled=vsMatch=psMatch=true;
    {Scope s(&d,false,8,12);}{Scope s(&d,true,9,12);}assert(!d.reads&&!d.writes);
    {Scope s(&d,true,8,12);assert(s.changed&&memcmp(original,d.ps,128)!=0);}
    assert(d.writes==2&&memcmp(original,d.ps,128)==0&&applied==1);
    d.writes=0;d.failFirst=true;
    {Scope s(&d,true,8,12);assert(!s.changed);assert(memcmp(original,d.ps,128)==0);}
    assert(d.writes==2&&rejected==1);
    d.writes=0;d.failFirst=false;d.failRestore=true;
    {Scope s(&d,true,8,12);}assert(fault&&restoreFailures==1);
    unsigned writes=d.writes;{Scope s(&d,true,8,12);}assert(d.writes==writes);
    puts("PASS: default OFF, narrow gate, normal/failed upload restore, fault latch");
}
