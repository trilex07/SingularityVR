#define NOMINMAX
#include <windows.h>
#include <d3d9.h>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <cassert>
static void Log(const char*,...) {}
#include "../shadow_pass_trace.h"
struct MockVS : IDirect3DVertexShader9 {
    ULONG refs=1;unsigned calls=0;UINT required=8;bool fail=false;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID,void**) override {return E_NOINTERFACE;}
    ULONG STDMETHODCALLTYPE AddRef() override {return ++refs;}
    ULONG STDMETHODCALLTYPE Release() override {return --refs;}
    HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9**) override {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE GetFunction(void* dest,UINT* size) override {
        ++calls;if(fail)return E_FAIL;
        if(!dest){*size=required;return S_OK;}
        assert(*size>=required);memset(dest,0,required);*size=required;return S_OK;
    }
};
int main() {
    using namespace shadowtrace;
    assert(!active && !pending);
    assert(Before(nullptr,4,535,535,0)==-1 && draws==0); // OFF makes zero device queries.
    NoteCopy(nullptr,nullptr,nullptr,nullptr,D3DTEXF_NONE,S_OK);
    NoteClear(nullptr,0,D3DCLEAR_ZBUFFER,0,1,0,S_OK);
    assert(copyCount==0 && clearCount==0 && operations==0); // OFF bypasses queries and retention.
    active=true;
    for(unsigned i=0;i<129;++i)NoteCopy(nullptr,nullptr,nullptr,nullptr,D3DTEXF_NONE,S_OK);
    assert(copyCount==128 && overflow==1 && copies[127].op==128);Release();assert(copyCount==0 && overflow==0 && operations==0);
    MockVS vs;vs.AddRef();groups[0]={};groups[0].key.vs=&vs;count=1;
    Release();assert(vs.refs==1 && count==0 && !active);
    FILE* f=nullptr;assert(tmpfile_s(&f)==0 && f);
    WriteShader(f,&vs,false);assert(vs.calls==2); // Size negotiation precedes bounded copy.
    vs.calls=0;vs.required=65536;WriteShader(f,&vs,false);assert(vs.calls==1);
    vs.calls=0;vs.required=7;WriteShader(f,&vs,false);assert(vs.calls==1);
    vs.calls=0;vs.fail=true;WriteShader(f,&vs,false);assert(vs.calls==1);
    fclose(f);
    puts("PASS: trace OFF issues no queries; COM refs released; bounded GetFunction size negotiation and failures");
}
