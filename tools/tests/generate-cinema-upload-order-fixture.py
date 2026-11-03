"""Compile the actual upload hook prefix through its native-camera return."""
from pathlib import Path
import sys,subprocess
root=Path(__file__).resolve().parents[2]
s=(subprocess.check_output(['git','show',sys.argv[2]+':spikes/view_matrix/d3d9.cpp'],cwd=root).decode('utf-8-sig') if len(sys.argv)>2 else (root/'spikes/view_matrix/d3d9.cpp').read_text(encoding='utf-8-sig'))
start=s.index('HRESULT STDMETHODCALLTYPE Hook_SetVSConstF')
a=s.index('    if (!Rd(g_rtIsScene) || g_boundShaderLayout.shadow)',start)
b=s.index('    // The TMD shader declares BoneMatrices',a)
prefix=s[a:b]
text=r'''#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#undef assert
#define assert(x) do {if(!(x)){fprintf(stderr,"FAIL: %s\n",#x);std::exit(1);}}while(0)
volatile LONG g_rtIsScene=1,g_hudConstsWritten=0,g_hudXformLive=0;
struct {bool shadow=false;} g_boundShaderLayout;
bool g_inHudRemap=false,g_camMatrixBound=true,native=true;
int forwarded=0;const float* forwardedData=nullptr;
LONG Rd(volatile LONG& v){return v;}
void InvalidateCamMats(){}
void Log(const char*,...){}
bool CutsceneNativeCameraArmed(){return native;}
HRESULT g_origSetVSConstF(void*,UINT,const float* p,UINT){++forwarded;forwardedData=p;return S_OK;}
HRESULT upload(UINT startReg,const float* data,UINT vec4Count){void* dev=nullptr;
'''+prefix+r'''
return S_FALSE;}
void reset(){g_hudConstsWritten=g_hudXformLive=0;g_rtIsScene=1;g_boundShaderLayout.shadow=false;g_inHudRemap=false;forwarded=0;}
int main(){
float data[64]{};reset();assert(upload(5,data,4)==S_OK);
assert(g_hudConstsWritten==1 && g_hudXformLive==1);assert(forwarded==1 && forwardedData==data);
reset();upload(0,data,16);assert(!g_hudXformLive);
reset();g_rtIsScene=0;upload(5,data,4);assert(!g_hudXformLive);
reset();g_boundShaderLayout.shadow=true;upload(5,data,4);assert(!g_hudXformLive);
reset();g_inHudRemap=true;upload(5,data,4);assert(!g_hudXformLive);
reset();native=false;assert(upload(5,data,4)==S_FALSE && g_hudXformLive);
puts("PASS actual hook prefix: cinema GFx witness precedes native return; constants forwarded untouched; scene/shadow/bulk/self gates preserved");}
'''
Path(sys.argv[1]).write_text(text,encoding='utf-8')
