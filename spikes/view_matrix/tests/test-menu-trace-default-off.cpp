// Execute the production census gates; OFF must not even dereference a device.
#define NOMINMAX
#include <windows.h>
#include <d3d9.h>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <cassert>
#include <cstdio>
static LONG Rd(volatile LONG& x){return InterlockedCompareExchange(&x,0,0);}
static unsigned tokenReads=0,logs=0;static unsigned long long token=1;
static unsigned long long VrMenuStateToken(){++tokenReads;return token;}
static void Log(const char*,...){++logs;}
static volatile LONG g_vrMenuProviderDrawing=0,g_rtIsScene=0,g_vrMenuSourceBound=0;
static struct {bool shadow=false;} g_boundShaderLayout;
static HRESULT(WINAPI* g_origDrawPrim)(IDirect3DDevice9*,D3DPRIMITIVETYPE,UINT,UINT)=nullptr;
static HRESULT(WINAPI* g_origDrawIndexedPrim)(IDirect3DDevice9*,D3DPRIMITIVETYPE,INT,UINT,UINT,UINT,UINT)=nullptr;
static HRESULT(WINAPI* g_origDrawPrimUP)(IDirect3DDevice9*,D3DPRIMITIVETYPE,UINT,const void*,UINT)=nullptr;
static HRESULT(WINAPI* g_origDrawIndexedPrimUP)(IDirect3DDevice9*,D3DPRIMITIVETYPE,UINT,UINT,UINT,const void*,D3DFORMAT,const void*,UINT)=nullptr;
#include "../menu_draw_trace.h"
int main(){
    assert(Rd(g_menuDrawTrace)==0);g_menuGroups[0].count=7;
    for(unsigned i=0;i<10000;++i){token=i+1;MenuDrawTraceFrame();
        MenuDrawTraceRecord(nullptr,1,D3DPT_TRIANGLELIST,1,3,0,0,0,nullptr,0);}
    assert(!tokenReads&&!logs&&!g_menuTraceFrame&&g_menuGroups[0].count==7);
    InterlockedExchange(&g_menuDrawTrace,1);MenuDrawTraceFrame();
    assert(tokenReads==1&&g_menuTraceFrame==1&&!g_menuGroups[0].count);
    // Opt-in still advances state and reaches the original pre-sampling gate.
    for(unsigned i=0;i<8;++i){MenuDrawTraceFrame();
        MenuDrawTraceRecord(nullptr,1,D3DPT_TRIANGLELIST,1,3,0,0,0,nullptr,0);}
    assert(tokenReads==17&&g_menuTraceFrame==9);
    InterlockedExchange(&g_menuDrawTrace,0);++token;
    for(unsigned i=0;i<10000;++i){MenuDrawTraceFrame();
        MenuDrawTraceRecord(nullptr,1,D3DPT_TRIANGLELIST,1,3,0,0,0,nullptr,0);}
    assert(tokenReads==17&&g_menuTraceFrame==9&&!logs);
    puts("PASS production MenuDrawTrace: default OFF, 20000 disabled calls without device/menu-state work; explicit ON and OFF transition");
}
